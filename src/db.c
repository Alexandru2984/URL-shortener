#include "db.h"
#include "utils.h"
#include <cjson/cJSON.h>
#include <limits.h>
#include <openssl/crypto.h>
#include <openssl/hmac.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

sqlite3 *db = NULL;

static pthread_mutex_t rate_cleanup_mutex = PTHREAD_MUTEX_INITIALIZER;
static time_t last_rate_cleanup = 0;
static const char *analytics_hmac_key = NULL;

#define ANALYTICS_HASH_PREFIX "hmac-sha256:v1:"
#define ANALYTICS_HASH_LEN 80U

static int hash_visitor_ip(const char *ip, char *hash_out, size_t hash_out_len) {
    if (!analytics_hmac_key || !analytics_hmac_key[0] || !ip || !ip[0] ||
        hash_out_len < ANALYTICS_HASH_LEN || strlen(analytics_hmac_key) > (size_t)INT_MAX) {
        return -1;
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    if (!HMAC(EVP_sha256(), analytics_hmac_key, (int)strlen(analytics_hmac_key),
              (const unsigned char *)ip, strlen(ip), digest, &digest_len) || digest_len != 32U) {
        OPENSSL_cleanse(digest, sizeof(digest));
        return -1;
    }

    static const char hex[] = "0123456789abcdef";
    const size_t prefix_len = sizeof(ANALYTICS_HASH_PREFIX) - 1U;
    memcpy(hash_out, ANALYTICS_HASH_PREFIX, prefix_len);
    for (size_t i = 0; i < digest_len; i++) {
        hash_out[prefix_len + i * 2U] = hex[digest[i] >> 4U];
        hash_out[prefix_len + i * 2U + 1U] = hex[digest[i] & 0x0fU];
    }
    hash_out[prefix_len + digest_len * 2U] = '\0';
    OPENSSL_cleanse(digest, sizeof(digest));
    return 0;
}

static int exec_sql(const char *label, const char *sql) {
    char *error_message = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &error_message);
    if (rc != SQLITE_OK) {
        log_error("Database %s failed: %s", label, error_message ? error_message : sqlite3_errmsg(db));
        sqlite3_free(error_message);
        return -1;
    }
    return 0;
}

static int table_has_column(const char *table, const char *column) {
    char sql[96];
    int sql_len = snprintf(sql, sizeof(sql), "PRAGMA table_info(%s);", table);
    if (sql_len < 0 || (size_t)sql_len >= sizeof(sql)) return 0;

    sqlite3_stmt *statement = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) return 0;

    int found = 0;
    while (sqlite3_step(statement) == SQLITE_ROW) {
        const unsigned char *name = sqlite3_column_text(statement, 1);
        if (name && strcmp((const char *)name, column) == 0) {
            found = 1;
            break;
        }
    }
    sqlite3_finalize(statement);
    return found;
}

static int ensure_table_column(const char *table, const char *column, const char *definition) {
    if (table_has_column(table, column)) return 0;

    char sql[192];
    int sql_len = snprintf(sql, sizeof(sql), "ALTER TABLE %s ADD COLUMN %s %s;", table, column, definition);
    if (sql_len < 0 || (size_t)sql_len >= sizeof(sql)) return -1;
    return exec_sql("schema migration", sql);
}

static int looks_like_legacy_salted_hash(const char *value) {
    if (!value || strlen(value) != 97U || value[32] != ':') return 0;
    for (size_t i = 0; i < 97U; i++) {
        if (i == 32U) continue;
        char c = value[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
              (c >= 'A' && c <= 'F'))) {
            return 0;
        }
    }
    return 1;
}

static int migrate_plaintext_passwords(void) {
    static const char select_sql[] =
        "SELECT id, password FROM links WHERE password IS NOT NULL AND password != '';";
    static const char update_sql[] = "UPDATE links SET password = ? WHERE id = ?;";
    sqlite3_stmt *select_statement = NULL;
    sqlite3_stmt *update_statement = NULL;
    int migrated = 0;

    if (exec_sql("begin password migration", "BEGIN IMMEDIATE;") != 0) return -1;
    if (sqlite3_prepare_v2(db, select_sql, -1, &select_statement, NULL) != SQLITE_OK ||
        sqlite3_prepare_v2(db, update_sql, -1, &update_statement, NULL) != SQLITE_OK) {
        log_error("Could not prepare password migration: %s", sqlite3_errmsg(db));
        sqlite3_finalize(select_statement);
        sqlite3_finalize(update_statement);
        exec_sql("rollback password migration", "ROLLBACK;");
        return -1;
    }

    int rc;
    while ((rc = sqlite3_step(select_statement)) == SQLITE_ROW) {
        sqlite3_int64 id = sqlite3_column_int64(select_statement, 0);
        const unsigned char *stored_value = sqlite3_column_text(select_statement, 1);
        if (!stored_value) continue;

        const char *stored_password = (const char *)stored_value;
        if (strncmp(stored_password, "pbkdf2-sha256$", 14U) == 0 ||
            looks_like_legacy_salted_hash(stored_password)) {
            continue;
        }

        char password_hash[128];
        if (hash_password(stored_password, password_hash, sizeof(password_hash)) != 0) {
            log_error("Could not hash a legacy password during migration");
            sqlite3_finalize(select_statement);
            sqlite3_finalize(update_statement);
            exec_sql("rollback password migration", "ROLLBACK;");
            return -1;
        }

        sqlite3_reset(update_statement);
        sqlite3_clear_bindings(update_statement);
        sqlite3_bind_text(update_statement, 1, password_hash, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(update_statement, 2, id);
        OPENSSL_cleanse(password_hash, sizeof(password_hash));
        if (sqlite3_step(update_statement) != SQLITE_DONE) {
            log_error("Could not update a migrated password: %s", sqlite3_errmsg(db));
            sqlite3_finalize(select_statement);
            sqlite3_finalize(update_statement);
            exec_sql("rollback password migration", "ROLLBACK;");
            return -1;
        }
        migrated++;
    }
    sqlite3_finalize(select_statement);
    sqlite3_finalize(update_statement);
    if (rc != SQLITE_DONE || exec_sql("commit password migration", "COMMIT;") != 0) {
        exec_sql("rollback password migration", "ROLLBACK;");
        return -1;
    }

    if (migrated > 0) log_message("Migrated %d plaintext link password(s) to PBKDF2", migrated);
    return 0;
}

static int anonymize_existing_visits(void) {
    static const char select_sql[] =
        "SELECT id, ip FROM visits WHERE ip IS NOT NULL AND ip NOT LIKE '" ANALYTICS_HASH_PREFIX "%';";
    static const char update_sql[] = "UPDATE visits SET ip = ?, user_agent = NULL WHERE id = ?;";
    sqlite3_stmt *select_statement = NULL;
    sqlite3_stmt *update_statement = NULL;
    int migrated = 0;

    if (exec_sql("begin analytics migration", "BEGIN IMMEDIATE;") != 0) return -1;
    if (sqlite3_prepare_v2(db, select_sql, -1, &select_statement, NULL) != SQLITE_OK ||
        sqlite3_prepare_v2(db, update_sql, -1, &update_statement, NULL) != SQLITE_OK) {
        log_error("Could not prepare analytics migration: %s", sqlite3_errmsg(db));
        sqlite3_finalize(select_statement);
        sqlite3_finalize(update_statement);
        exec_sql("rollback analytics migration", "ROLLBACK;");
        return -1;
    }

    int rc;
    while ((rc = sqlite3_step(select_statement)) == SQLITE_ROW) {
        sqlite3_int64 id = sqlite3_column_int64(select_statement, 0);
        const unsigned char *stored_ip = sqlite3_column_text(select_statement, 1);
        char ip_hash[ANALYTICS_HASH_LEN];
        const char *replacement = NULL;
        if (stored_ip && hash_visitor_ip((const char *)stored_ip, ip_hash, sizeof(ip_hash)) == 0) {
            replacement = ip_hash;
        }

        sqlite3_reset(update_statement);
        sqlite3_clear_bindings(update_statement);
        if (replacement) sqlite3_bind_text(update_statement, 1, replacement, -1, SQLITE_TRANSIENT);
        else sqlite3_bind_null(update_statement, 1);
        sqlite3_bind_int64(update_statement, 2, id);
        if (sqlite3_step(update_statement) != SQLITE_DONE) {
            log_error("Could not anonymize an analytics row: %s", sqlite3_errmsg(db));
            sqlite3_finalize(select_statement);
            sqlite3_finalize(update_statement);
            exec_sql("rollback analytics migration", "ROLLBACK;");
            return -1;
        }
        migrated++;
    }
    sqlite3_finalize(select_statement);
    sqlite3_finalize(update_statement);
    if (rc != SQLITE_DONE ||
        exec_sql("clear stored user agents", "UPDATE visits SET user_agent = NULL WHERE user_agent IS NOT NULL;") != 0 ||
        exec_sql("sync clicks_count",
                 "UPDATE links SET clicks_count = (SELECT COUNT(*) FROM visits v WHERE v.slug = links.slug) "
                 "WHERE clicks_count IS NULL OR clicks_count < (SELECT COUNT(*) FROM visits v WHERE v.slug = links.slug);") != 0 ||
        exec_sql("commit analytics migration", "COMMIT;") != 0) {
        exec_sql("rollback analytics migration", "ROLLBACK;");
        return -1;
    }

    if (migrated > 0) {
        log_message("Anonymized %d stored visitor identifier(s)", migrated);
    }
    if (!analytics_hmac_key || !analytics_hmac_key[0]) {
        log_error("ANALYTICS_HMAC_KEY is not configured; unique visitor counts will be unavailable");
    }
    return 0;
}

int init_db(const char *db_path, const char *analytics_key) {
    if (!db_path || !db_path[0]) return -1;

    int open_flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
    if (sqlite3_open_v2(db_path, &db, open_flags, NULL) != SQLITE_OK) {
        log_error("Cannot open database: %s", db ? sqlite3_errmsg(db) : "unknown error");
        close_db();
        return -1;
    }
    sqlite3_extended_result_codes(db, 1);
    sqlite3_busy_timeout(db, 5000);
    sqlite3_db_config(db, SQLITE_DBCONFIG_DEFENSIVE, 1, NULL);
    analytics_hmac_key = analytics_key;

    if (exec_sql("enable WAL", "PRAGMA journal_mode=WAL;") != 0 ||
        exec_sql("set journal size limit", "PRAGMA journal_size_limit=67108864;") != 0 ||
        exec_sql("set synchronous mode", "PRAGMA synchronous=NORMAL;") != 0 ||
        exec_sql("enable foreign keys", "PRAGMA foreign_keys=ON;") != 0 ||
        exec_sql("disable trusted schema", "PRAGMA trusted_schema=OFF;") != 0 ||
        exec_sql("set temp storage", "PRAGMA temp_store=MEMORY;") != 0) {
        close_db();
        return -1;
    }

    static const char links_sql[] =
        "CREATE TABLE IF NOT EXISTS links ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "slug TEXT UNIQUE NOT NULL,"
        "url TEXT NOT NULL,"
        "expires_at DATETIME,"
        "password TEXT,"
        "max_clicks INTEGER DEFAULT 0,"
        "activates_at DATETIME,"
        "is_active INTEGER DEFAULT 1,"
        "title TEXT,"
        "tags TEXT,"
        "mobile_url TEXT,"
        "idempotency_key TEXT,"
        "clicks_count INTEGER DEFAULT 0,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP);";
    static const char visits_sql[] =
        "CREATE TABLE IF NOT EXISTS visits ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "slug TEXT NOT NULL,"
        "ip TEXT,"
        "user_agent TEXT,"
        "device_type TEXT DEFAULT 'unknown',"
        "referrer_domain TEXT DEFAULT 'direct',"
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";
    static const char rate_limit_sql[] =
        "CREATE TABLE IF NOT EXISTS rate_limit ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "ip TEXT NOT NULL,"
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";
    static const char tokens_sql[] =
        "CREATE TABLE IF NOT EXISTS api_tokens ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "token_prefix TEXT NOT NULL,"
        "token_hash TEXT UNIQUE NOT NULL,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "expires_at DATETIME,"
        "last_used_at DATETIME,"
        "usage_count INTEGER DEFAULT 0,"
        "is_active INTEGER DEFAULT 1);";
    static const char audit_sql[] =
        "CREATE TABLE IF NOT EXISTS audit_logs ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "event_type TEXT NOT NULL,"
        "actor TEXT NOT NULL,"
        "target TEXT,"
        "ip_hash TEXT,"
        "details TEXT,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP);";

    if (exec_sql("create links table", links_sql) != 0 ||
        ensure_table_column("links", "expires_at", "DATETIME") != 0 ||
        ensure_table_column("links", "password", "TEXT") != 0 ||
        ensure_table_column("links", "max_clicks", "INTEGER DEFAULT 0") != 0 ||
        ensure_table_column("links", "activates_at", "DATETIME") != 0 ||
        ensure_table_column("links", "is_active", "INTEGER DEFAULT 1") != 0 ||
        ensure_table_column("links", "title", "TEXT") != 0 ||
        ensure_table_column("links", "tags", "TEXT") != 0 ||
        ensure_table_column("links", "mobile_url", "TEXT") != 0 ||
        ensure_table_column("links", "idempotency_key", "TEXT") != 0 ||
        ensure_table_column("links", "clicks_count", "INTEGER DEFAULT 0") != 0 ||
        exec_sql("create visits table", visits_sql) != 0 ||
        ensure_table_column("visits", "device_type", "TEXT DEFAULT 'unknown'") != 0 ||
        ensure_table_column("visits", "referrer_domain", "TEXT DEFAULT 'direct'") != 0 ||
        exec_sql("create rate limit table", rate_limit_sql) != 0 ||
        exec_sql("create api tokens table", tokens_sql) != 0 ||
        exec_sql("create audit logs table", audit_sql) != 0 ||
        exec_sql("create visits index", "CREATE INDEX IF NOT EXISTS idx_visits_slug ON visits(slug);") != 0 ||
        exec_sql("create visits ts index", "CREATE INDEX IF NOT EXISTS idx_visits_slug_ts ON visits(slug, timestamp);") != 0 ||
        exec_sql("create rate limit index", "CREATE INDEX IF NOT EXISTS idx_rate_limit_ip_ts ON rate_limit(ip, timestamp);") != 0 ||
        exec_sql("create expiration index", "CREATE INDEX IF NOT EXISTS idx_links_expires_at ON links(expires_at);") != 0 ||
        exec_sql("create idempotency index", "CREATE INDEX IF NOT EXISTS idx_links_idemp ON links(idempotency_key);") != 0 ||
        exec_sql("create audit ts index", "CREATE INDEX IF NOT EXISTS idx_audit_created_at ON audit_logs(created_at DESC);") != 0 ||
        migrate_plaintext_passwords() != 0 ||
        anonymize_existing_visits() != 0) {
        close_db();
        return -1;
    }
    return 0;
}

int insert_link_ex(const char *slug, const char *url, const struct link_options *opts) {
    if (!slug || !url) return -1;

    int ttl_hours = opts ? opts->ttl_hours : 0;
    int max_clicks = opts ? opts->max_clicks : 0;
    int activates_in_hours = opts ? opts->activates_in_hours : 0;
    const char *password = opts ? opts->password : NULL;
    const char *title = (opts && opts->title && opts->title[0]) ? opts->title : NULL;
    const char *tags = (opts && opts->tags && opts->tags[0]) ? opts->tags : NULL;
    const char *mobile_url = (opts && opts->mobile_url && opts->mobile_url[0]) ? opts->mobile_url : NULL;
    const char *idemp_key = (opts && opts->idempotency_key && opts->idempotency_key[0]) ? opts->idempotency_key : NULL;

    if (ttl_hours < 0 || max_clicks < 0 || activates_in_hours < 0) return -1;

    static const char sql[] =
        "INSERT INTO links (slug, url, expires_at, activates_at, password, max_clicks, title, tags, mobile_url, idempotency_key, is_active, clicks_count) "
        "VALUES (?, ?, "
        "CASE WHEN ? > 0 THEN datetime('now', '+' || ? || ' hours') ELSE NULL END, "
        "CASE WHEN ? > 0 THEN datetime('now', '+' || ? || ' hours') ELSE NULL END, "
        "?, ?, ?, ?, ?, ?, 1, 0);";

    sqlite3_stmt *statement = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) return -1;

    char password_hash[128];
    const char *stored_password = NULL;
    if (password && password[0] != '\0') {
        if (hash_password(password, password_hash, sizeof(password_hash)) != 0) {
            sqlite3_finalize(statement);
            return -1;
        }
        stored_password = password_hash;
    }

    sqlite3_bind_text(statement, 1, slug, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, url, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(statement, 3, ttl_hours);
    sqlite3_bind_int(statement, 4, ttl_hours);
    sqlite3_bind_int(statement, 5, activates_in_hours);
    sqlite3_bind_int(statement, 6, activates_in_hours);
    if (stored_password) sqlite3_bind_text(statement, 7, stored_password, -1, SQLITE_TRANSIENT);
    else sqlite3_bind_null(statement, 7);
    sqlite3_bind_int(statement, 8, max_clicks);
    if (title) sqlite3_bind_text(statement, 9, title, -1, SQLITE_TRANSIENT);
    else sqlite3_bind_null(statement, 9);
    if (tags) sqlite3_bind_text(statement, 10, tags, -1, SQLITE_TRANSIENT);
    else sqlite3_bind_null(statement, 10);
    if (mobile_url) sqlite3_bind_text(statement, 11, mobile_url, -1, SQLITE_TRANSIENT);
    else sqlite3_bind_null(statement, 11);
    if (idemp_key) sqlite3_bind_text(statement, 12, idemp_key, -1, SQLITE_TRANSIENT);
    else sqlite3_bind_null(statement, 12);

    int rc = sqlite3_step(statement);
    OPENSSL_cleanse(password_hash, sizeof(password_hash));
    sqlite3_finalize(statement);
    return rc == SQLITE_DONE ? 0 : -1;
}

int find_slug_by_idempotency_key(const char *idempotency_key, char *slug_out, size_t slug_out_len) {
    if (!idempotency_key || !idempotency_key[0] || !slug_out || slug_out_len == 0) return -1;

    static const char sql[] =
        "SELECT slug FROM links WHERE idempotency_key = ? "
        "AND created_at >= datetime('now', '-24 hours') LIMIT 1;";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, idempotency_key, -1, SQLITE_TRANSIENT);

    int found = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *s = sqlite3_column_text(stmt, 0);
        if (s && strlen((const char *)s) < slug_out_len) {
            snprintf(slug_out, slug_out_len, "%s", (const char *)s);
            found = 0;
        }
    }
    sqlite3_finalize(stmt);
    return found;
}

int insert_link(const char *slug, const char *url, int ttl_hours, const char *password) {
    struct link_options opts;
    memset(&opts, 0, sizeof(opts));
    opts.ttl_hours = ttl_hours;
    opts.password = password;
    return insert_link_ex(slug, url, &opts);
}

int update_link(const char *slug, const char *new_url, const char *title,
                const char *tags, int ttl_hours, int max_clicks,
                int is_active, const char *new_password) {
    if (!slug || !new_url || max_clicks < 0) return -1;

    // ttl_hours: -1 = keep current expires_at, 0 = remove expiry, >0 = set new expiry from now
    // new_password: NULL = keep current password, "" = clear password, non-empty = set new password
    static const char sql[] =
        "UPDATE links SET "
        "url = ?, "
        "title = ?, "
        "tags = ?, "
        "expires_at = CASE WHEN ? = 0 THEN NULL WHEN ? > 0 THEN datetime('now', '+' || ? || ' hours') ELSE expires_at END, "
        "max_clicks = ?, "
        "is_active = ?, "
        "password = CASE WHEN ? = 1 THEN ? ELSE password END "
        "WHERE slug = ?;";

    sqlite3_stmt *statement = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) return -1;

    char password_hash[128];
    const char *password_to_store = NULL;
    int update_password = 0;
    if (new_password != NULL) {
        update_password = 1;
        if (new_password[0] != '\0') {
            if (hash_password(new_password, password_hash, sizeof(password_hash)) != 0) {
                sqlite3_finalize(statement);
                return -1;
            }
            password_to_store = password_hash;
        }
    }

    sqlite3_bind_text(statement, 1, new_url, -1, SQLITE_TRANSIENT);
    if (title && title[0]) sqlite3_bind_text(statement, 2, title, -1, SQLITE_TRANSIENT);
    else sqlite3_bind_null(statement, 2);
    if (tags && tags[0]) sqlite3_bind_text(statement, 3, tags, -1, SQLITE_TRANSIENT);
    else sqlite3_bind_null(statement, 3);
    sqlite3_bind_int(statement, 4, ttl_hours);
    sqlite3_bind_int(statement, 5, ttl_hours);
    sqlite3_bind_int(statement, 6, ttl_hours);
    sqlite3_bind_int(statement, 7, max_clicks);
    sqlite3_bind_int(statement, 8, is_active ? 1 : 0);
    sqlite3_bind_int(statement, 9, update_password);
    if (password_to_store) sqlite3_bind_text(statement, 10, password_to_store, -1, SQLITE_TRANSIENT);
    else sqlite3_bind_null(statement, 10);
    sqlite3_bind_text(statement, 11, slug, -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(statement);
    int changes = sqlite3_changes(db);
    OPENSSL_cleanse(password_hash, sizeof(password_hash));
    sqlite3_finalize(statement);
    return (rc == SQLITE_DONE && changes == 1) ? 0 : -1;
}

static int copy_column_text(sqlite3_stmt *statement, int column, char *output, size_t output_len) {
    if (!output || output_len == 0) return -1;
    const unsigned char *value = sqlite3_column_text(statement, column);
    int bytes = sqlite3_column_bytes(statement, column);
    if (!value || bytes < 0 || (size_t)bytes >= output_len) return -1;
    memcpy(output, value, (size_t)bytes);
    output[bytes] = '\0';
    return 0;
}

int get_link_for_device(const char *slug, const char *device_type,
                        char *url_out, size_t url_out_len, int *requires_password) {
    if (!slug || !url_out || url_out_len == 0 || !requires_password) return -1;
    *requires_password = 0;

    static const char sql[] =
        "SELECT url, password, "
        "expires_at IS NOT NULL AND datetime('now') > expires_at, "
        "activates_at IS NOT NULL AND datetime('now') < activates_at, "
        "COALESCE(is_active, 1), "
        "COALESCE(max_clicks, 0), "
        "COALESCE(clicks_count, 0), "
        "mobile_url "
        "FROM links WHERE slug = ?;";
    sqlite3_stmt *statement = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(statement, 1, slug, -1, SQLITE_TRANSIENT);

    int result = -1;
    if (sqlite3_step(statement) == SQLITE_ROW) {
        int is_expired = sqlite3_column_int(statement, 2);
        int is_scheduled = sqlite3_column_int(statement, 3);
        int is_active = sqlite3_column_int(statement, 4);
        int max_clicks = sqlite3_column_int(statement, 5);
        int clicks_count = sqlite3_column_int(statement, 6);

        if (!is_active) {
            result = -6;
        } else if (is_scheduled) {
            result = -5;
        } else if (is_expired) {
            result = -2;
        } else if (max_clicks > 0 && clicks_count >= max_clicks) {
            result = -3;
        } else {
            const unsigned char *password = sqlite3_column_text(statement, 1);
            if (password && password[0] != '\0') {
                *requires_password = 1;
                result = 0;
            } else {
                const unsigned char *mob = sqlite3_column_text(statement, 7);
                if (device_type && strcmp(device_type, "mobile") == 0 && mob && mob[0] != '\0') {
                    result = copy_column_text(statement, 7, url_out, url_out_len);
                } else {
                    result = copy_column_text(statement, 0, url_out, url_out_len);
                }
            }
        }
    }
    sqlite3_finalize(statement);
    return result;
}

int get_link(const char *slug, char *url_out, size_t url_out_len, int *requires_password) {
    return get_link_for_device(slug, "desktop", url_out, url_out_len, requires_password);
}

static void opportunistic_password_rehash(const char *slug, const char *previous_hash,
                                          const char *password) {
    if (!password_needs_rehash(previous_hash)) return;

    char replacement_hash[128];
    if (hash_password(password, replacement_hash, sizeof(replacement_hash)) != 0) {
        log_error("Could not upgrade legacy password hash for %s", slug);
        return;
    }

    static const char sql[] = "UPDATE links SET password = ? WHERE slug = ? AND password = ?;";
    sqlite3_stmt *statement = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) {
        OPENSSL_cleanse(replacement_hash, sizeof(replacement_hash));
        log_error("Could not prepare password hash upgrade for %s", slug);
        return;
    }
    sqlite3_bind_text(statement, 1, replacement_hash, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, slug, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, previous_hash, -1, SQLITE_TRANSIENT);
    if (sqlite3_step(statement) != SQLITE_DONE) {
        log_error("Could not upgrade password hash for %s", slug);
    }
    OPENSSL_cleanse(replacement_hash, sizeof(replacement_hash));
    sqlite3_finalize(statement);
}

int get_link_with_password(const char *slug, const char *password_in,
                           char *url_out, size_t url_out_len) {
    if (!slug || !password_in || !url_out || url_out_len == 0) return -1;

    static const char sql[] =
        "SELECT url, password, "
        "expires_at IS NOT NULL AND datetime('now') > expires_at, "
        "activates_at IS NOT NULL AND datetime('now') < activates_at, "
        "COALESCE(is_active, 1), "
        "COALESCE(max_clicks, 0), "
        "COALESCE(clicks_count, 0) "
        "FROM links WHERE slug = ?;";
    sqlite3_stmt *statement = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(statement, 1, slug, -1, SQLITE_TRANSIENT);

    int result = -1;
    char previous_hash[128];
    previous_hash[0] = '\0';
    if (sqlite3_step(statement) == SQLITE_ROW) {
        int is_expired = sqlite3_column_int(statement, 2);
        int is_scheduled = sqlite3_column_int(statement, 3);
        int is_active = sqlite3_column_int(statement, 4);
        int max_clicks = sqlite3_column_int(statement, 5);
        int clicks_count = sqlite3_column_int(statement, 6);

        if (!is_active) {
            result = -6;
        } else if (is_scheduled) {
            result = -5;
        } else if (is_expired) {
            result = -2;
        } else if (max_clicks > 0 && clicks_count >= max_clicks) {
            result = -3;
        } else {
            const unsigned char *stored_password = sqlite3_column_text(statement, 1);
            if (!stored_password || !verify_password(password_in, (const char *)stored_password)) {
                result = -4;
            } else {
                size_t password_len = strnlen((const char *)stored_password, sizeof(previous_hash));
                if (password_len >= sizeof(previous_hash) ||
                    copy_column_text(statement, 0, url_out, url_out_len) != 0) {
                    result = -1;
                } else {
                    memcpy(previous_hash, stored_password, password_len + 1U);
                    result = 0;
                }
            }
        }
    }
    sqlite3_finalize(statement);

    if (result == 0) opportunistic_password_rehash(slug, previous_hash, password_in);
    OPENSSL_cleanse(previous_hash, sizeof(previous_hash));
    return result;
}

int record_visit_ex(const char *slug, const char *ip,
                    const char *device_type, const char *referrer_domain) {
    if (!slug || !slug[0]) return -1;

    static const char insert_visit_sql[] =
        "INSERT INTO visits (slug, ip, device_type, referrer_domain) VALUES (?, ?, ?, ?);";
    static const char update_clicks_sql[] =
        "UPDATE links SET clicks_count = COALESCE(clicks_count, 0) + 1 WHERE slug = ?;";

    sqlite3_stmt *visit_stmt = NULL;
    sqlite3_stmt *clicks_stmt = NULL;
    if (sqlite3_prepare_v2(db, insert_visit_sql, -1, &visit_stmt, NULL) != SQLITE_OK) return -1;

    char ip_hash[ANALYTICS_HASH_LEN];
    const char *stored_ip = NULL;
    if (hash_visitor_ip(ip, ip_hash, sizeof(ip_hash)) == 0) stored_ip = ip_hash;

    const char *dev = (device_type && device_type[0]) ? device_type : "unknown";
    const char *ref = (referrer_domain && referrer_domain[0]) ? referrer_domain : "direct";

    sqlite3_bind_text(visit_stmt, 1, slug, -1, SQLITE_TRANSIENT);
    if (stored_ip) sqlite3_bind_text(visit_stmt, 2, stored_ip, -1, SQLITE_TRANSIENT);
    else sqlite3_bind_null(visit_stmt, 2);
    sqlite3_bind_text(visit_stmt, 3, dev, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(visit_stmt, 4, ref, -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(visit_stmt);
    sqlite3_finalize(visit_stmt);
    if (rc != SQLITE_DONE) return -1;

    if (sqlite3_prepare_v2(db, update_clicks_sql, -1, &clicks_stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(clicks_stmt, 1, slug, -1, SQLITE_TRANSIENT);
        (void)sqlite3_step(clicks_stmt);
        sqlite3_finalize(clicks_stmt);
    }
    return 0;
}

int record_visit(const char *slug, const char *ip) {
    return record_visit_ex(slug, ip, "unknown", "direct");
}

int get_stats(const char *slug, int *total_visits, int *unique_visits) {
    if (!slug || !total_visits || !unique_visits) return -1;
    *total_visits = 0;
    *unique_visits = 0;

    static const char sql[] =
        "SELECT COALESCE((SELECT clicks_count FROM links WHERE slug = ?), 0), "
        "COUNT(*), COUNT(DISTINCT ip) FROM visits WHERE slug = ?;";
    sqlite3_stmt *statement = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(statement, 1, slug, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, slug, -1, SQLITE_TRANSIENT);
    int result = -1;
    if (sqlite3_step(statement) == SQLITE_ROW) {
        sqlite3_int64 link_clicks = sqlite3_column_int64(statement, 0);
        sqlite3_int64 visit_rows = sqlite3_column_int64(statement, 1);
        sqlite3_int64 unique = sqlite3_column_int64(statement, 2);
        sqlite3_int64 total = link_clicks > visit_rows ? link_clicks : visit_rows;
        *total_visits = total > INT_MAX ? INT_MAX : (int)total;
        *unique_visits = unique > INT_MAX ? INT_MAX : (int)unique;
        result = 0;
    }
    sqlite3_finalize(statement);
    return result;
}

char *get_detailed_stats_json(const char *slug) {
    if (!slug || !slug[0]) return NULL;

    static const char meta_sql[] =
        "SELECT slug, url, created_at, expires_at, activates_at, "
        "COALESCE(is_active, 1), COALESCE(max_clicks, 0), COALESCE(clicks_count, 0), "
        "(password IS NOT NULL AND password != ''), title, tags, "
        "expires_at IS NOT NULL AND datetime('now') > expires_at, "
        "activates_at IS NOT NULL AND datetime('now') < activates_at "
        "FROM links WHERE slug = ?;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, meta_sql, -1, &stmt, NULL) != SQLITE_OK) return NULL;
    sqlite3_bind_text(stmt, 1, slug, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return NULL;
    }

    int is_active = sqlite3_column_int(stmt, 5);
    int max_clicks = sqlite3_column_int(stmt, 6);
    int clicks_count = sqlite3_column_int(stmt, 7);
    int has_password = sqlite3_column_int(stmt, 8);
    int is_expired = sqlite3_column_int(stmt, 11);
    int is_scheduled = sqlite3_column_int(stmt, 12);

    const char *status = "active";
    if (!is_active) status = "disabled";
    else if (is_expired) status = "expired";
    else if (is_scheduled) status = "scheduled";
    else if (max_clicks > 0 && clicks_count >= max_clicks) status = "limit_reached";

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        sqlite3_finalize(stmt);
        return NULL;
    }

    const char *created_at = (const char *)sqlite3_column_text(stmt, 2);
    const char *expires_at = (const char *)sqlite3_column_text(stmt, 3);
    const char *activates_at = (const char *)sqlite3_column_text(stmt, 4);
    const char *title = (const char *)sqlite3_column_text(stmt, 9);
    const char *tags = (const char *)sqlite3_column_text(stmt, 10);
    const char *url = (const char *)sqlite3_column_text(stmt, 1);

    cJSON_AddStringToObject(root, "slug", slug);
    cJSON_AddStringToObject(root, "status", status);
    cJSON_AddStringToObject(root, "created_at", created_at ? created_at : "");
    if (expires_at) cJSON_AddStringToObject(root, "expires_at", expires_at);
    else cJSON_AddNullToObject(root, "expires_at");
    if (activates_at) cJSON_AddStringToObject(root, "activates_at", activates_at);
    else cJSON_AddNullToObject(root, "activates_at");
    if (title) cJSON_AddStringToObject(root, "title", title);
    else cJSON_AddNullToObject(root, "title");
    if (tags) cJSON_AddStringToObject(root, "tags", tags);
    else cJSON_AddNullToObject(root, "tags");
    cJSON_AddBoolToObject(root, "has_password", has_password);
    cJSON_AddBoolToObject(root, "is_active", is_active);
    cJSON_AddNumberToObject(root, "max_clicks", max_clicks);

    if (!has_password && url) {
        char domain[256];
        classify_referrer(url, NULL, domain, sizeof(domain));
        cJSON_AddStringToObject(root, "target_domain", domain);
    } else {
        cJSON_AddNullToObject(root, "target_domain");
    }
    sqlite3_finalize(stmt);

    int total_visits = 0;
    int unique_visits = 0;
    (void)get_stats(slug, &total_visits, &unique_visits);
    cJSON_AddNumberToObject(root, "total_visits", total_visits);
    cJSON_AddNumberToObject(root, "unique_visitors", unique_visits);
    if (max_clicks > 0) {
        int remaining = max_clicks > total_visits ? (max_clicks - total_visits) : 0;
        cJSON_AddNumberToObject(root, "remaining_clicks", remaining);
    } else {
        cJSON_AddNullToObject(root, "remaining_clicks");
    }

    // Daily visits over last 14 days
    cJSON *daily = cJSON_CreateArray();
    static const char daily_sql[] =
        "SELECT substr(timestamp, 1, 10) AS day, COUNT(*), COUNT(DISTINCT ip) "
        "FROM visits WHERE slug = ? AND timestamp >= datetime('now', '-14 days') "
        "GROUP BY day ORDER BY day ASC;";
    if (daily && sqlite3_prepare_v2(db, daily_sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, slug, -1, SQLITE_TRANSIENT);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *day = (const char *)sqlite3_column_text(stmt, 0);
            int d_visits = sqlite3_column_int(stmt, 1);
            int d_unique = sqlite3_column_int(stmt, 2);
            cJSON *d_obj = cJSON_CreateObject();
            if (d_obj && day) {
                cJSON_AddStringToObject(d_obj, "day", day);
                cJSON_AddNumberToObject(d_obj, "visits", d_visits);
                cJSON_AddNumberToObject(d_obj, "unique", d_unique);
                cJSON_AddItemToArray(daily, d_obj);
            } else {
                cJSON_Delete(d_obj);
            }
        }
        sqlite3_finalize(stmt);
    }
    cJSON_AddItemToObject(root, "daily", daily ? daily : cJSON_CreateArray());

    // Device breakdown
    cJSON *devices = cJSON_CreateObject();
    static const char dev_sql[] =
        "SELECT COALESCE(device_type, 'unknown'), COUNT(*) FROM visits WHERE slug = ? GROUP BY device_type;";
    if (devices && sqlite3_prepare_v2(db, dev_sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, slug, -1, SQLITE_TRANSIENT);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *dtype = (const char *)sqlite3_column_text(stmt, 0);
            int count = sqlite3_column_int(stmt, 1);
            if (dtype) cJSON_AddNumberToObject(devices, dtype, count);
        }
        sqlite3_finalize(stmt);
    }
    cJSON_AddItemToObject(root, "devices", devices ? devices : cJSON_CreateObject());

    // Top referrer domains
    cJSON *referrers = cJSON_CreateArray();
    static const char ref_sql[] =
        "SELECT COALESCE(referrer_domain, 'direct') AS ref, COUNT(*) AS cnt "
        "FROM visits WHERE slug = ? GROUP BY ref ORDER BY cnt DESC LIMIT 8;";
    if (referrers && sqlite3_prepare_v2(db, ref_sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, slug, -1, SQLITE_TRANSIENT);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *ref = (const char *)sqlite3_column_text(stmt, 0);
            int cnt = sqlite3_column_int(stmt, 1);
            cJSON *r_obj = cJSON_CreateObject();
            if (r_obj && ref) {
                cJSON_AddStringToObject(r_obj, "domain", ref);
                cJSON_AddNumberToObject(r_obj, "visits", cnt);
                cJSON_AddItemToArray(referrers, r_obj);
            } else {
                cJSON_Delete(r_obj);
            }
        }
        sqlite3_finalize(stmt);
    }
    cJSON_AddItemToObject(root, "referrers", referrers ? referrers : cJSON_CreateArray());

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

char *get_link_inspect_json(const char *slug) {
    if (!slug || !slug[0]) return NULL;

    static const char sql[] =
        "SELECT slug, url, created_at, expires_at, activates_at, "
        "COALESCE(is_active, 1), COALESCE(max_clicks, 0), COALESCE(clicks_count, 0), "
        "(password IS NOT NULL AND password != ''), title, tags, "
        "expires_at IS NOT NULL AND datetime('now') > expires_at, "
        "activates_at IS NOT NULL AND datetime('now') < activates_at "
        "FROM links WHERE slug = ?;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return NULL;
    sqlite3_bind_text(stmt, 1, slug, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return NULL;
    }

    const char *url = (const char *)sqlite3_column_text(stmt, 1);
    const char *created_at = (const char *)sqlite3_column_text(stmt, 2);
    const char *expires_at = (const char *)sqlite3_column_text(stmt, 3);
    const char *activates_at = (const char *)sqlite3_column_text(stmt, 4);
    int is_active = sqlite3_column_int(stmt, 5);
    int max_clicks = sqlite3_column_int(stmt, 6);
    int clicks_count = sqlite3_column_int(stmt, 7);
    int has_password = sqlite3_column_int(stmt, 8);
    const char *title = (const char *)sqlite3_column_text(stmt, 9);
    const char *tags = (const char *)sqlite3_column_text(stmt, 10);
    int is_expired = sqlite3_column_int(stmt, 11);
    int is_scheduled = sqlite3_column_int(stmt, 12);

    const char *status = "active";
    if (!is_active) status = "disabled";
    else if (is_expired) status = "expired";
    else if (is_scheduled) status = "scheduled";
    else if (max_clicks > 0 && clicks_count >= max_clicks) status = "limit_reached";

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        sqlite3_finalize(stmt);
        return NULL;
    }

    cJSON_AddStringToObject(root, "slug", slug);
    cJSON_AddStringToObject(root, "status", status);
    cJSON_AddStringToObject(root, "created_at", created_at ? created_at : "");
    if (expires_at) cJSON_AddStringToObject(root, "expires_at", expires_at);
    else cJSON_AddNullToObject(root, "expires_at");
    if (activates_at) cJSON_AddStringToObject(root, "activates_at", activates_at);
    else cJSON_AddNullToObject(root, "activates_at");
    if (title) cJSON_AddStringToObject(root, "title", title);
    else cJSON_AddNullToObject(root, "title");
    if (tags) cJSON_AddStringToObject(root, "tags", tags);
    else cJSON_AddNullToObject(root, "tags");
    cJSON_AddBoolToObject(root, "has_password", has_password);
    cJSON_AddNumberToObject(root, "max_clicks", max_clicks);
    cJSON_AddNumberToObject(root, "total_visits", clicks_count);

    // Never disclose target URL on password-protected or inactive links
    if (!has_password && strcmp(status, "active") == 0 && url) {
        char domain[256];
        classify_referrer(url, NULL, domain, sizeof(domain));
        cJSON_AddStringToObject(root, "target_url", url);
        cJSON_AddStringToObject(root, "target_domain", domain);
    } else {
        cJSON_AddNullToObject(root, "target_url");
        cJSON_AddNullToObject(root, "target_domain");
    }

    sqlite3_finalize(stmt);
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

char *get_bio_collection_json(const char *profile) {
    if (!profile || !is_valid_slug(profile, 24U)) return NULL;

    char tag_query[32];
    int n = snprintf(tag_query, sizeof(tag_query), "bio-%s", profile);
    if (n < 0 || (size_t)n >= sizeof(tag_query)) return NULL;

    static const char sql[] =
        "SELECT slug, url, COALESCE(title, slug), COALESCE(clicks_count, 0), created_at "
        "FROM links "
        "WHERE COALESCE(is_active, 1) = 1 "
        "AND (password IS NULL OR password = '') "
        "AND (expires_at IS NULL OR datetime('now') <= expires_at) "
        "AND (activates_at IS NULL OR datetime('now') >= activates_at) "
        "AND (COALESCE(max_clicks, 0) = 0 OR COALESCE(clicks_count, 0) < max_clicks) "
        "AND (',' || COALESCE(tags, '') || ',' LIKE '%,' || ? || ',%' "
        "     OR ',' || COALESCE(tags, '') || ',' LIKE '%,' || ? || ',%') "
        "ORDER BY created_at DESC LIMIT 50;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return NULL;
    sqlite3_bind_text(stmt, 1, tag_query, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, profile, -1, SQLITE_TRANSIENT);

    cJSON *root = cJSON_CreateObject();
    cJSON *items = cJSON_CreateArray();
    if (!root || !items) {
        cJSON_Delete(root);
        cJSON_Delete(items);
        sqlite3_finalize(stmt);
        return NULL;
    }

    cJSON_AddStringToObject(root, "profile", profile);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *slug = (const char *)sqlite3_column_text(stmt, 0);
        const char *url = (const char *)sqlite3_column_text(stmt, 1);
        const char *title = (const char *)sqlite3_column_text(stmt, 2);
        int clicks = sqlite3_column_int(stmt, 3);
        const char *created = (const char *)sqlite3_column_text(stmt, 4);

        cJSON *obj = cJSON_CreateObject();
        if (obj && slug && url) {
            cJSON_AddStringToObject(obj, "slug", slug);
            cJSON_AddStringToObject(obj, "url", url);
            cJSON_AddStringToObject(obj, "title", title ? title : slug);
            cJSON_AddNumberToObject(obj, "clicks", clicks);
            cJSON_AddStringToObject(obj, "created_at", created ? created : "");
            cJSON_AddItemToArray(items, obj);
        } else {
            cJSON_Delete(obj);
        }
    }
    sqlite3_finalize(stmt);
    cJSON_AddItemToObject(root, "links", items);
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

static void query_db_metrics(int *total_links, int *active_links, int *expired_links,
                             int *disabled_links, int *total_visits, int *unique_visitors,
                             int *active_tokens, int *audit_events, long *db_bytes) {
    *total_links = *active_links = *expired_links = *disabled_links = 0;
    *total_visits = *unique_visitors = *active_tokens = *audit_events = 0;
    *db_bytes = 0;

    static const char sql[] =
        "SELECT "
        "(SELECT COUNT(*) FROM links), "
        "(SELECT COUNT(*) FROM links WHERE COALESCE(is_active, 1) = 1 AND (expires_at IS NULL OR datetime('now') <= expires_at)), "
        "(SELECT COUNT(*) FROM links WHERE expires_at IS NOT NULL AND datetime('now') > expires_at), "
        "(SELECT COUNT(*) FROM links WHERE COALESCE(is_active, 1) = 0), "
        "(SELECT COUNT(*) FROM visits), "
        "(SELECT COUNT(DISTINCT ip) FROM visits), "
        "(SELECT COUNT(*) FROM api_tokens WHERE is_active = 1), "
        "(SELECT COUNT(*) FROM audit_logs);";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            *total_links = sqlite3_column_int(stmt, 0);
            *active_links = sqlite3_column_int(stmt, 1);
            *expired_links = sqlite3_column_int(stmt, 2);
            *disabled_links = sqlite3_column_int(stmt, 3);
            *total_visits = sqlite3_column_int(stmt, 4);
            *unique_visitors = sqlite3_column_int(stmt, 5);
            *active_tokens = sqlite3_column_int(stmt, 6);
            *audit_events = sqlite3_column_int(stmt, 7);
        }
        sqlite3_finalize(stmt);
    }

    long page_count = 0, page_size = 4096;
    if (sqlite3_prepare_v2(db, "PRAGMA page_count;", -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) page_count = (long)sqlite3_column_int64(stmt, 0);
        sqlite3_finalize(stmt);
    }
    if (sqlite3_prepare_v2(db, "PRAGMA page_size;", -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) page_size = (long)sqlite3_column_int64(stmt, 0);
        sqlite3_finalize(stmt);
    }
    *db_bytes = page_count * page_size;
}

char *get_prometheus_metrics(long uptime_seconds) {
    int total_links, active_links, expired_links, disabled_links;
    int total_visits, unique_visitors, active_tokens, audit_events;
    long db_bytes;
    query_db_metrics(&total_links, &active_links, &expired_links, &disabled_links,
                     &total_visits, &unique_visitors, &active_tokens, &audit_events, &db_bytes);

    char *buf = malloc(2048U);
    if (!buf) return NULL;
    snprintf(buf, 2048U,
             "# HELP shortener_uptime_seconds Process uptime in seconds.\n"
             "# TYPE shortener_uptime_seconds gauge\n"
             "shortener_uptime_seconds %ld\n"
             "# HELP shortener_links_total Total stored short links.\n"
             "# TYPE shortener_links_total gauge\n"
             "shortener_links_total %d\n"
             "# HELP shortener_links_active Currently active short links.\n"
             "# TYPE shortener_links_active gauge\n"
             "shortener_links_active %d\n"
             "# HELP shortener_links_expired Expired links awaiting cleanup.\n"
             "# TYPE shortener_links_expired gauge\n"
             "shortener_links_expired %d\n"
             "# HELP shortener_links_disabled Administratively disabled links.\n"
             "# TYPE shortener_links_disabled gauge\n"
             "shortener_links_disabled %d\n"
             "# HELP shortener_visits_total Total recorded link visits.\n"
             "# TYPE shortener_visits_total counter\n"
             "shortener_visits_total %d\n"
             "# HELP shortener_unique_visitors_total Total unique HMAC visitor identifiers.\n"
             "# TYPE shortener_unique_visitors_total gauge\n"
             "shortener_unique_visitors_total %d\n"
             "# HELP shortener_tokens_active Active scoped creator tokens.\n"
             "# TYPE shortener_tokens_active gauge\n"
             "shortener_tokens_active %d\n"
             "# HELP shortener_audit_events_total Total security audit log entries.\n"
             "# TYPE shortener_audit_events_total counter\n"
             "shortener_audit_events_total %d\n"
             "# HELP shortener_db_size_bytes SQLite database size in bytes.\n"
             "# TYPE shortener_db_size_bytes gauge\n"
             "shortener_db_size_bytes %ld\n",
             uptime_seconds, total_links, active_links, expired_links, disabled_links,
             total_visits, unique_visitors, active_tokens, audit_events, db_bytes);
    return buf;
}

char *get_system_diagnostics_json(long uptime_seconds) {
    int total_links, active_links, expired_links, disabled_links;
    int total_visits, unique_visitors, active_tokens, audit_events;
    long db_bytes;
    query_db_metrics(&total_links, &active_links, &expired_links, &disabled_links,
                     &total_visits, &unique_visitors, &active_tokens, &audit_events, &db_bytes);

    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;
    cJSON_AddNumberToObject(root, "uptime_seconds", (double)uptime_seconds);
    cJSON_AddNumberToObject(root, "total_links", total_links);
    cJSON_AddNumberToObject(root, "active_links", active_links);
    cJSON_AddNumberToObject(root, "expired_links", expired_links);
    cJSON_AddNumberToObject(root, "disabled_links", disabled_links);
    cJSON_AddNumberToObject(root, "total_visits", total_visits);
    cJSON_AddNumberToObject(root, "unique_visitors", unique_visitors);
    cJSON_AddNumberToObject(root, "active_tokens", active_tokens);
    cJSON_AddNumberToObject(root, "audit_events", audit_events);
    cJSON_AddNumberToObject(root, "db_size_bytes", (double)db_bytes);
    cJSON_AddStringToObject(root, "sqlite_version", sqlite3_libversion());
    cJSON_AddStringToObject(root, "c_standard", "C17 (ISO/IEC 9899:2018)");
    cJSON_AddStringToObject(root, "hardening", "PIE, Full RELRO, Stack Protector Strong, FORTIFY_SOURCE=3, Strict CSP");

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

static void cleanup_stale_rate_limit_entries(void) {
    time_t now = time(NULL);
    if (now == (time_t)-1) return;

    pthread_mutex_lock(&rate_cleanup_mutex);
    if (now - last_rate_cleanup >= 60) {
        if (exec_sql("clean stale rate limit rows",
                     "DELETE FROM rate_limit WHERE timestamp <= datetime('now', '-1 minute');") == 0) {
            last_rate_cleanup = now;
        }
    }
    pthread_mutex_unlock(&rate_cleanup_mutex);
}

int check_rate_limit(const char *ip, int max_requests_per_min) {
    if (!ip || !ip[0] || max_requests_per_min < 1) return -1;
    cleanup_stale_rate_limit_entries();

    static const char sql[] =
        "INSERT INTO rate_limit (ip) "
        "SELECT ? WHERE (SELECT COUNT(*) FROM rate_limit "
        "WHERE ip = ? AND timestamp > datetime('now', '-1 minute')) < ?;";
    sqlite3_stmt *statement = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(statement, 1, ip, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, ip, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(statement, 3, max_requests_per_min);
    int rc = sqlite3_step(statement);
    int inserted = sqlite3_changes(db);
    sqlite3_finalize(statement);
    return rc == SQLITE_DONE && inserted == 1 ? 0 : -1;
}

int delete_link(const char *slug) {
    if (!slug || exec_sql("begin link deletion", "BEGIN IMMEDIATE;") != 0) return -1;

    sqlite3_stmt *visits_statement = NULL;
    sqlite3_stmt *link_statement = NULL;
    int result = -1;
    if (sqlite3_prepare_v2(db, "DELETE FROM visits WHERE slug = ?;", -1, &visits_statement, NULL) != SQLITE_OK ||
        sqlite3_prepare_v2(db, "DELETE FROM links WHERE slug = ?;", -1, &link_statement, NULL) != SQLITE_OK) {
        goto cleanup;
    }
    sqlite3_bind_text(visits_statement, 1, slug, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(link_statement, 1, slug, -1, SQLITE_TRANSIENT);
    if (sqlite3_step(visits_statement) != SQLITE_DONE || sqlite3_step(link_statement) != SQLITE_DONE) goto cleanup;
    result = sqlite3_changes(db) == 1 ? 0 : -1;

cleanup:
    sqlite3_finalize(visits_statement);
    sqlite3_finalize(link_statement);
    if (result == 0 && exec_sql("commit link deletion", "COMMIT;") == 0) return 0;
    exec_sql("rollback link deletion", "ROLLBACK;");
    return -1;
}

int cleanup_expired_links(void) {
    if (exec_sql("begin expiration cleanup", "BEGIN IMMEDIATE;") != 0) return -1;

    static const char delete_visits_sql[] =
        "DELETE FROM visits WHERE slug IN (SELECT slug FROM links "
        "WHERE expires_at IS NOT NULL AND datetime('now') > expires_at);";
    static const char delete_links_sql[] =
        "DELETE FROM links WHERE expires_at IS NOT NULL AND datetime('now') > expires_at;";
    if (exec_sql("delete expired visits", delete_visits_sql) != 0 ||
        exec_sql("delete expired links", delete_links_sql) != 0) {
        exec_sql("rollback expiration cleanup", "ROLLBACK;");
        return -1;
    }

    int deleted = sqlite3_changes(db);
    if (exec_sql("commit expiration cleanup", "COMMIT;") != 0) {
        exec_sql("rollback expiration cleanup", "ROLLBACK;");
        return -1;
    }
    return deleted;
}

int cleanup_old_analytics(int retention_days) {
    if (retention_days < 1 || retention_days > 3650) return -1;

    static const char sql[] =
        "DELETE FROM visits WHERE timestamp < datetime('now', '-' || ? || ' days');";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_int(stmt, 1, retention_days);
    int rc = sqlite3_step(stmt);
    int deleted = sqlite3_changes(db);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE ? deleted : -1;
}

int create_api_token(const char *name, int ttl_days,
                     char *raw_token_out, size_t raw_token_len) {
    if (!name || !name[0] || ttl_days < 0 || !raw_token_out || raw_token_len < 42U) return -1;

    char prefix[16];
    char token_hash[65];
    if (generate_api_token(raw_token_out, raw_token_len, prefix, sizeof(prefix)) != 0 ||
        hash_api_token(raw_token_out, token_hash, sizeof(token_hash)) != 0) {
        return -1;
    }

    static const char sql[] =
        "INSERT INTO api_tokens (name, token_prefix, token_hash, expires_at, is_active) "
        "VALUES (?, ?, ?, CASE WHEN ? > 0 THEN datetime('now', '+' || ? || ' days') ELSE NULL END, 1);";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        OPENSSL_cleanse(raw_token_out, raw_token_len);
        return -1;
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, prefix, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, token_hash, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, ttl_days);
    sqlite3_bind_int(stmt, 5, ttl_days);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        OPENSSL_cleanse(raw_token_out, raw_token_len);
        return -1;
    }
    return 0;
}

int verify_creator_token(const char *raw_token, char *token_name_out, size_t name_len) {
    if (!raw_token || strncmp(raw_token, "csh_", 4U) != 0) return 0;

    char token_hash[65];
    if (hash_api_token(raw_token, token_hash, sizeof(token_hash)) != 0) return 0;

    static const char select_sql[] =
        "SELECT id, name FROM api_tokens WHERE token_hash = ? AND is_active = 1 "
        "AND (expires_at IS NULL OR datetime('now') <= expires_at);";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, select_sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_text(stmt, 1, token_hash, -1, SQLITE_TRANSIENT);

    int valid = 0;
    sqlite3_int64 token_id = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        token_id = sqlite3_column_int64(stmt, 0);
        const char *name = (const char *)sqlite3_column_text(stmt, 1);
        if (token_name_out && name_len > 0) {
            snprintf(token_name_out, name_len, "token:%s", name ? name : "scoped");
        }
        valid = 1;
    }
    sqlite3_finalize(stmt);

    if (valid) {
        static const char update_sql[] =
            "UPDATE api_tokens SET last_used_at = CURRENT_TIMESTAMP, usage_count = usage_count + 1 WHERE id = ?;";
        if (sqlite3_prepare_v2(db, update_sql, -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_int64(stmt, 1, token_id);
            (void)sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
    return valid;
}

char *list_api_tokens_json(void) {
    static const char sql[] =
        "SELECT id, name, token_prefix, created_at, expires_at, last_used_at, usage_count, is_active "
        "FROM api_tokens ORDER BY created_at DESC LIMIT 100;";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return NULL;

    cJSON *arr = cJSON_CreateArray();
    if (!arr) {
        sqlite3_finalize(stmt);
        return NULL;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        cJSON *obj = cJSON_CreateObject();
        if (!obj) break;
        const char *name = (const char *)sqlite3_column_text(stmt, 1);
        const char *prefix = (const char *)sqlite3_column_text(stmt, 2);
        const char *created = (const char *)sqlite3_column_text(stmt, 3);
        const char *expires = (const char *)sqlite3_column_text(stmt, 4);
        const char *last_used = (const char *)sqlite3_column_text(stmt, 5);

        cJSON_AddNumberToObject(obj, "id", sqlite3_column_int(stmt, 0));
        cJSON_AddStringToObject(obj, "name", name ? name : "");
        cJSON_AddStringToObject(obj, "token_prefix", prefix ? prefix : "");
        cJSON_AddStringToObject(obj, "created_at", created ? created : "");
        if (expires) cJSON_AddStringToObject(obj, "expires_at", expires);
        else cJSON_AddNullToObject(obj, "expires_at");
        if (last_used) cJSON_AddStringToObject(obj, "last_used_at", last_used);
        else cJSON_AddNullToObject(obj, "last_used_at");
        cJSON_AddNumberToObject(obj, "usage_count", sqlite3_column_int(stmt, 6));
        cJSON_AddBoolToObject(obj, "is_active", sqlite3_column_int(stmt, 7));
        cJSON_AddItemToArray(arr, obj);
    }
    sqlite3_finalize(stmt);
    char *json = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);
    return json;
}

int revoke_api_token(int token_id) {
    if (token_id < 1) return -1;
    static const char sql[] = "DELETE FROM api_tokens WHERE id = ?;";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_int(stmt, 1, token_id);
    int rc = sqlite3_step(stmt);
    int changes = sqlite3_changes(db);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE && changes == 1) ? 0 : -1;
}

int record_audit_event(const char *event_type, const char *actor,
                       const char *target, const char *ip, const char *details) {
    if (!event_type || !actor) return -1;

    static const char sql[] =
        "INSERT INTO audit_logs (event_type, actor, target, ip_hash, details) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    char ip_hash[ANALYTICS_HASH_LEN];
    const char *stored_ip = NULL;
    if (ip && hash_visitor_ip(ip, ip_hash, sizeof(ip_hash)) == 0) {
        stored_ip = ip_hash;
    }

    sqlite3_bind_text(stmt, 1, event_type, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, actor, -1, SQLITE_TRANSIENT);
    if (target && target[0]) sqlite3_bind_text(stmt, 3, target, -1, SQLITE_TRANSIENT);
    else sqlite3_bind_null(stmt, 3);
    if (stored_ip) sqlite3_bind_text(stmt, 4, stored_ip, -1, SQLITE_TRANSIENT);
    else sqlite3_bind_null(stmt, 4);
    if (details && details[0]) sqlite3_bind_text(stmt, 5, details, -1, SQLITE_TRANSIENT);
    else sqlite3_bind_null(stmt, 5);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE ? 0 : -1;
}

char *get_audit_logs_json(int limit) {
    if (limit < 1 || limit > 500) limit = 100;
    static const char sql[] =
        "SELECT id, event_type, actor, target, details, created_at "
        "FROM audit_logs ORDER BY id DESC LIMIT ?;";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return NULL;
    sqlite3_bind_int(stmt, 1, limit);

    cJSON *arr = cJSON_CreateArray();
    if (!arr) {
        sqlite3_finalize(stmt);
        return NULL;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        cJSON *obj = cJSON_CreateObject();
        if (!obj) break;
        const char *ev = (const char *)sqlite3_column_text(stmt, 1);
        const char *actor = (const char *)sqlite3_column_text(stmt, 2);
        const char *target = (const char *)sqlite3_column_text(stmt, 3);
        const char *details = (const char *)sqlite3_column_text(stmt, 4);
        const char *created = (const char *)sqlite3_column_text(stmt, 5);

        cJSON_AddNumberToObject(obj, "id", sqlite3_column_int(stmt, 0));
        cJSON_AddStringToObject(obj, "event_type", ev ? ev : "");
        cJSON_AddStringToObject(obj, "actor", actor ? actor : "");
        if (target) cJSON_AddStringToObject(obj, "target", target);
        else cJSON_AddNullToObject(obj, "target");
        if (details) cJSON_AddStringToObject(obj, "details", details);
        else cJSON_AddNullToObject(obj, "details");
        cJSON_AddStringToObject(obj, "created_at", created ? created : "");
        cJSON_AddItemToArray(arr, obj);
    }
    sqlite3_finalize(stmt);
    char *json = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);
    return json;
}

char *query_admin_links_json(const char *search, const char *tag,
                             const char *status_filter, int limit, int offset) {
    if (limit < 1 || limit > 1000) limit = 1000;
    if (offset < 0) offset = 0;

    static const char sql[] =
        "SELECT l.slug, l.url, l.created_at, l.expires_at, l.activates_at, "
        "(l.password IS NOT NULL AND l.password != '') AS has_password, "
        "COALESCE(l.is_active, 1) AS is_active, "
        "COALESCE(l.max_clicks, 0) AS max_clicks, "
        "COALESCE(l.clicks_count, 0) AS total_visits, "
        "(SELECT COUNT(DISTINCT v.ip) FROM visits v WHERE v.slug = l.slug) AS unique_visits, "
        "l.title, l.tags, "
        "(l.expires_at IS NOT NULL AND datetime('now') > l.expires_at) AS is_expired, "
        "l.mobile_url "
        "FROM links l "
        "WHERE (? IS NULL OR l.slug LIKE '%' || ? || '%' OR l.url LIKE '%' || ? || '%' OR COALESCE(l.title, '') LIKE '%' || ? || '%') "
        "AND (? IS NULL OR ',' || COALESCE(l.tags, '') || ',' LIKE '%,' || ? || ',%') "
        "AND (? IS NULL "
        "     OR (? = 'active' AND COALESCE(l.is_active, 1) = 1 AND (l.expires_at IS NULL OR datetime('now') <= l.expires_at)) "
        "     OR (? = 'expired' AND l.expires_at IS NOT NULL AND datetime('now') > l.expires_at) "
        "     OR (? = 'disabled' AND COALESCE(l.is_active, 1) = 0)) "
        "ORDER BY l.created_at DESC LIMIT ? OFFSET ?;";

    sqlite3_stmt *statement = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) return NULL;

    const char *q = (search && search[0]) ? search : NULL;
    const char *t = (tag && tag[0]) ? tag : NULL;
    const char *st = (status_filter && status_filter[0]) ? status_filter : NULL;

    for (int i = 1; i <= 4; i++) {
        if (q) sqlite3_bind_text(statement, i, q, -1, SQLITE_TRANSIENT);
        else sqlite3_bind_null(statement, i);
    }
    for (int i = 5; i <= 6; i++) {
        if (t) sqlite3_bind_text(statement, i, t, -1, SQLITE_TRANSIENT);
        else sqlite3_bind_null(statement, i);
    }
    for (int i = 7; i <= 10; i++) {
        if (st) sqlite3_bind_text(statement, i, st, -1, SQLITE_TRANSIENT);
        else sqlite3_bind_null(statement, i);
    }
    sqlite3_bind_int(statement, 11, limit);
    sqlite3_bind_int(statement, 12, offset);

    cJSON *links = cJSON_CreateArray();
    if (!links) {
        sqlite3_finalize(statement);
        return NULL;
    }

    int success = 1;
    while (sqlite3_step(statement) == SQLITE_ROW) {
        cJSON *item = cJSON_CreateObject();
        const char *slug = (const char *)sqlite3_column_text(statement, 0);
        const char *url = (const char *)sqlite3_column_text(statement, 1);
        const char *created = (const char *)sqlite3_column_text(statement, 2);
        const char *expires = (const char *)sqlite3_column_text(statement, 3);
        const char *activates = (const char *)sqlite3_column_text(statement, 4);
        const char *title = (const char *)sqlite3_column_text(statement, 10);
        const char *tags = (const char *)sqlite3_column_text(statement, 11);
        const char *mobile_url = (const char *)sqlite3_column_text(statement, 13);

        if (!item || !cJSON_AddStringToObject(item, "slug", slug ? slug : "") ||
            !cJSON_AddStringToObject(item, "url", url ? url : "") ||
            !(mobile_url ? cJSON_AddStringToObject(item, "mobile_url", mobile_url)
                         : cJSON_AddNullToObject(item, "mobile_url")) ||
            !cJSON_AddStringToObject(item, "created_at", created ? created : "") ||
            !(expires ? cJSON_AddStringToObject(item, "expires_at", expires)
                      : cJSON_AddNullToObject(item, "expires_at")) ||
            !(activates ? cJSON_AddStringToObject(item, "activates_at", activates)
                        : cJSON_AddNullToObject(item, "activates_at")) ||
            !cJSON_AddBoolToObject(item, "has_password", sqlite3_column_int(statement, 5)) ||
            !cJSON_AddBoolToObject(item, "is_active", sqlite3_column_int(statement, 6)) ||
            !cJSON_AddNumberToObject(item, "max_clicks", sqlite3_column_double(statement, 7)) ||
            !cJSON_AddNumberToObject(item, "total_visits", sqlite3_column_double(statement, 8)) ||
            !cJSON_AddNumberToObject(item, "unique_visitors", sqlite3_column_double(statement, 9)) ||
            !(title ? cJSON_AddStringToObject(item, "title", title)
                    : cJSON_AddNullToObject(item, "title")) ||
            !(tags ? cJSON_AddStringToObject(item, "tags", tags)
                   : cJSON_AddNullToObject(item, "tags")) ||
            !cJSON_AddBoolToObject(item, "is_expired", sqlite3_column_int(statement, 12))) {
            cJSON_Delete(item);
            success = 0;
            break;
        }
        cJSON_AddItemToArray(links, item);
    }
    sqlite3_finalize(statement);
    if (!success) {
        cJSON_Delete(links);
        return NULL;
    }

    char *json = cJSON_PrintUnformatted(links);
    cJSON_Delete(links);
    return json;
}

char *get_all_links_json(void) {
    return query_admin_links_json(NULL, NULL, NULL, 1000, 0);
}

char *export_links_csv(void) {
    static const char sql[] =
        "SELECT l.slug, l.url, COALESCE(l.title, ''), COALESCE(l.tags, ''), "
        "l.created_at, COALESCE(l.expires_at, ''), "
        "CASE WHEN l.password IS NOT NULL AND l.password != '' THEN 'yes' ELSE 'no' END, "
        "COALESCE(l.is_active, 1), COALESCE(l.max_clicks, 0), COALESCE(l.clicks_count, 0), "
        "(SELECT COUNT(DISTINCT v.ip) FROM visits v WHERE v.slug = l.slug) "
        "FROM links l ORDER BY l.created_at DESC LIMIT 2000;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return NULL;

    size_t cap = 16384U;
    size_t len = 0;
    char *csv = malloc(cap);
    if (!csv) {
        sqlite3_finalize(stmt);
        return NULL;
    }

    const char *header = "slug,url,title,tags,created_at,expires_at,has_password,is_active,max_clicks,total_visits,unique_visitors\r\n";
    size_t header_len = strlen(header);
    memcpy(csv, header, header_len + 1U);
    len = header_len;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        char slug_csv[128], url_csv[4200], title_csv[512], tags_csv[512];
        char created_csv[128], expires_csv[128], pwd_csv[32];

        sanitize_csv_field((const char *)sqlite3_column_text(stmt, 0), slug_csv, sizeof(slug_csv));
        sanitize_csv_field((const char *)sqlite3_column_text(stmt, 1), url_csv, sizeof(url_csv));
        sanitize_csv_field((const char *)sqlite3_column_text(stmt, 2), title_csv, sizeof(title_csv));
        sanitize_csv_field((const char *)sqlite3_column_text(stmt, 3), tags_csv, sizeof(tags_csv));
        sanitize_csv_field((const char *)sqlite3_column_text(stmt, 4), created_csv, sizeof(created_csv));
        sanitize_csv_field((const char *)sqlite3_column_text(stmt, 5), expires_csv, sizeof(expires_csv));
        sanitize_csv_field((const char *)sqlite3_column_text(stmt, 6), pwd_csv, sizeof(pwd_csv));

        int is_active = sqlite3_column_int(stmt, 7);
        int max_clicks = sqlite3_column_int(stmt, 8);
        int total_visits = sqlite3_column_int(stmt, 9);
        int unique_visits = sqlite3_column_int(stmt, 10);

        char row_buf[6000];
        int row_len = snprintf(row_buf, sizeof(row_buf), "%s,%s,%s,%s,%s,%s,%s,%d,%d,%d,%d\r\n",
                               slug_csv, url_csv, title_csv, tags_csv, created_csv, expires_csv,
                               pwd_csv, is_active, max_clicks, total_visits, unique_visits);
        if (row_len < 0 || (size_t)row_len >= sizeof(row_buf)) continue;

        if (len + (size_t)row_len + 1U > cap) {
            size_t new_cap = cap * 2U + (size_t)row_len;
            char *grown = realloc(csv, new_cap);
            if (!grown) {
                free(csv);
                sqlite3_finalize(stmt);
                return NULL;
            }
            csv = grown;
            cap = new_cap;
        }
        memcpy(csv + len, row_buf, (size_t)row_len);
        len += (size_t)row_len;
        csv[len] = '\0';
    }
    sqlite3_finalize(stmt);
    return csv;
}

void close_db(void) {
    if (!db) return;
    int rc = sqlite3_close_v2(db);
    if (rc != SQLITE_OK) log_error("Could not close database cleanly: %s", sqlite3_errstr(rc));
    db = NULL;
}
