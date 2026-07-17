#include "db.h"
#include "utils.h"
#include <cjson/cJSON.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

sqlite3 *db = NULL;

static pthread_mutex_t rate_cleanup_mutex = PTHREAD_MUTEX_INITIALIZER;
static time_t last_rate_cleanup = 0;

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

static int ensure_link_column(const char *column, const char *definition) {
    if (table_has_column("links", column)) return 0;

    char sql[160];
    int sql_len = snprintf(sql, sizeof(sql), "ALTER TABLE links ADD COLUMN %s %s;", column, definition);
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

int init_db(const char *db_path) {
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

    if (exec_sql("enable WAL", "PRAGMA journal_mode=WAL;") != 0 ||
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
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP);";
    static const char visits_sql[] =
        "CREATE TABLE IF NOT EXISTS visits ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "slug TEXT NOT NULL,"
        "ip TEXT,"
        "user_agent TEXT,"
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";
    static const char rate_limit_sql[] =
        "CREATE TABLE IF NOT EXISTS rate_limit ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "ip TEXT NOT NULL,"
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";

    if (exec_sql("create links table", links_sql) != 0 ||
        ensure_link_column("expires_at", "DATETIME") != 0 ||
        ensure_link_column("password", "TEXT") != 0 ||
        exec_sql("create visits table", visits_sql) != 0 ||
        exec_sql("create rate limit table", rate_limit_sql) != 0 ||
        exec_sql("create visits index", "CREATE INDEX IF NOT EXISTS idx_visits_slug ON visits(slug);") != 0 ||
        exec_sql("create rate limit index", "CREATE INDEX IF NOT EXISTS idx_rate_limit_ip_ts ON rate_limit(ip, timestamp);") != 0 ||
        exec_sql("create expiration index", "CREATE INDEX IF NOT EXISTS idx_links_expires_at ON links(expires_at);") != 0 ||
        migrate_plaintext_passwords() != 0) {
        close_db();
        return -1;
    }
    return 0;
}

int insert_link(const char *slug, const char *url, int ttl_hours, const char *password) {
    if (!slug || !url || ttl_hours < 0) return -1;

    const char *sql = ttl_hours > 0
        ? "INSERT INTO links (slug, url, expires_at, password) VALUES (?, ?, datetime('now', '+' || ? || ' hours'), ?);"
        : "INSERT INTO links (slug, url, password) VALUES (?, ?, ?);";
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
    if (ttl_hours > 0) {
        sqlite3_bind_int(statement, 3, ttl_hours);
        sqlite3_bind_text(statement, 4, stored_password, -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_text(statement, 3, stored_password, -1, SQLITE_TRANSIENT);
    }
    int rc = sqlite3_step(statement);
    sqlite3_finalize(statement);
    return rc == SQLITE_DONE ? 0 : -1;
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

int get_link(const char *slug, char *url_out, size_t url_out_len, int *requires_password) {
    if (!slug || !url_out || url_out_len == 0 || !requires_password) return -1;
    *requires_password = 0;

    static const char sql[] =
        "SELECT url, password, expires_at IS NOT NULL AND datetime('now') > expires_at "
        "FROM links WHERE slug = ?;";
    sqlite3_stmt *statement = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(statement, 1, slug, -1, SQLITE_TRANSIENT);

    int result = -1;
    if (sqlite3_step(statement) == SQLITE_ROW) {
        if (sqlite3_column_int(statement, 2) != 0) {
            result = -2;
        } else {
            const unsigned char *password = sqlite3_column_text(statement, 1);
            if (password && password[0] != '\0') {
                *requires_password = 1;
                result = 0;
            } else {
                result = copy_column_text(statement, 0, url_out, url_out_len);
            }
        }
    }
    sqlite3_finalize(statement);
    return result;
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
        log_error("Could not prepare password hash upgrade for %s", slug);
        return;
    }
    sqlite3_bind_text(statement, 1, replacement_hash, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, slug, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, previous_hash, -1, SQLITE_TRANSIENT);
    if (sqlite3_step(statement) != SQLITE_DONE) {
        log_error("Could not upgrade password hash for %s", slug);
    }
    sqlite3_finalize(statement);
}

int get_link_with_password(const char *slug, const char *password_in,
                           char *url_out, size_t url_out_len) {
    if (!slug || !password_in || !url_out || url_out_len == 0) return -1;

    static const char sql[] =
        "SELECT url, password, expires_at IS NOT NULL AND datetime('now') > expires_at "
        "FROM links WHERE slug = ?;";
    sqlite3_stmt *statement = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(statement, 1, slug, -1, SQLITE_TRANSIENT);

    int result = -1;
    char previous_hash[128];
    previous_hash[0] = '\0';
    if (sqlite3_step(statement) == SQLITE_ROW) {
        if (sqlite3_column_int(statement, 2) != 0) {
            result = -2;
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
    return result;
}

int record_visit(const char *slug, const char *ip, const char *user_agent) {
    static const char sql[] = "INSERT INTO visits (slug, ip, user_agent) VALUES (?, ?, ?);";
    sqlite3_stmt *statement = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(statement, 1, slug, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, ip, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, user_agent, -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(statement);
    sqlite3_finalize(statement);
    return rc == SQLITE_DONE ? 0 : -1;
}

int get_stats(const char *slug, int *total_visits, int *unique_visits) {
    if (!slug || !total_visits || !unique_visits) return -1;
    *total_visits = 0;
    *unique_visits = 0;

    static const char sql[] = "SELECT COUNT(*), COUNT(DISTINCT ip) FROM visits WHERE slug = ?;";
    sqlite3_stmt *statement = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(statement, 1, slug, -1, SQLITE_TRANSIENT);
    int result = -1;
    if (sqlite3_step(statement) == SQLITE_ROW) {
        sqlite3_int64 total = sqlite3_column_int64(statement, 0);
        sqlite3_int64 unique = sqlite3_column_int64(statement, 1);
        *total_visits = total > INT_MAX ? INT_MAX : (int)total;
        *unique_visits = unique > INT_MAX ? INT_MAX : (int)unique;
        result = 0;
    }
    sqlite3_finalize(statement);
    return result;
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

char *get_all_links_json(void) {
    static const char sql[] =
        "SELECT l.slug, l.url, l.created_at, l.expires_at, "
        "(l.password IS NOT NULL AND l.password != '') AS has_password, "
        "(SELECT COUNT(*) FROM visits v WHERE v.slug = l.slug) AS total_visits, "
        "(SELECT COUNT(DISTINCT v.ip) FROM visits v WHERE v.slug = l.slug) AS unique_visits "
        "FROM links l ORDER BY l.created_at DESC LIMIT 1000;";
    sqlite3_stmt *statement = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) return NULL;

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
        if (!item || !cJSON_AddStringToObject(item, "slug", slug ? slug : "") ||
            !cJSON_AddStringToObject(item, "url", url ? url : "") ||
            !cJSON_AddStringToObject(item, "created_at", created ? created : "") ||
            !(expires ? cJSON_AddStringToObject(item, "expires_at", expires)
                      : cJSON_AddNullToObject(item, "expires_at")) ||
            !cJSON_AddBoolToObject(item, "has_password", sqlite3_column_int(statement, 4)) ||
            !cJSON_AddNumberToObject(item, "total_visits", sqlite3_column_double(statement, 5)) ||
            !cJSON_AddNumberToObject(item, "unique_visitors", sqlite3_column_double(statement, 6))) {
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

void close_db(void) {
    if (!db) return;
    int rc = sqlite3_close_v2(db);
    if (rc != SQLITE_OK) log_error("Could not close database cleanly: %s", sqlite3_errstr(rc));
    db = NULL;
}
