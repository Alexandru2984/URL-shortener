#include "db.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int failures = 0;

#define CHECK(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "check failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            failures++; \
        } \
    } while (0)

static void cleanup_test_files(const char *directory) {
    char path[512];
    snprintf(path, sizeof(path), "%s/data/shortener.db", directory);
    unlink(path);
    snprintf(path, sizeof(path), "%s/data/shortener.db-wal", directory);
    unlink(path);
    snprintf(path, sizeof(path), "%s/data/shortener.db-shm", directory);
    unlink(path);
    snprintf(path, sizeof(path), "%s/logs/access.log", directory);
    unlink(path);
    snprintf(path, sizeof(path), "%s/logs/error.log", directory);
    unlink(path);
    snprintf(path, sizeof(path), "%s/data", directory);
    rmdir(path);
    snprintf(path, sizeof(path), "%s/logs", directory);
    rmdir(path);
    rmdir(directory);
}

int main(void) {
    char directory[64] = "/tmp/shortener-db-test.XXXXXX";
    char old_directory[512];
    char db_path[128];
    char data_path[96];
    char logs_path[96];
    CHECK(getcwd(old_directory, sizeof(old_directory)) != NULL);
    int temporary_file = mkstemp(directory);
    CHECK(temporary_file >= 0);
    if (temporary_file >= 0) {
        CHECK(close(temporary_file) == 0);
        CHECK(unlink(directory) == 0);
    }
    CHECK(mkdir(directory, 0700) == 0);
    snprintf(data_path, sizeof(data_path), "%s/data", directory);
    snprintf(logs_path, sizeof(logs_path), "%s/logs", directory);
    snprintf(db_path, sizeof(db_path), "%s/shortener.db", data_path);
    CHECK(mkdir(data_path, 0700) == 0);
    CHECK(mkdir(logs_path, 0700) == 0);

    sqlite3 *legacy_db = NULL;
    CHECK(sqlite3_open(db_path, &legacy_db) == SQLITE_OK);
    CHECK(sqlite3_exec(legacy_db,
                       "CREATE TABLE links (id INTEGER PRIMARY KEY AUTOINCREMENT, slug TEXT UNIQUE NOT NULL, "
                       "url TEXT NOT NULL, created_at DATETIME DEFAULT CURRENT_TIMESTAMP, expires_at DATETIME, password TEXT);"
                       "CREATE TABLE visits (id INTEGER PRIMARY KEY AUTOINCREMENT, slug TEXT NOT NULL, ip TEXT, user_agent TEXT, timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);"
                       "INSERT INTO links (slug, url, password) VALUES ('legacy', 'https://example.test/legacy', 'old-password');"
                       "INSERT INTO visits (slug, ip, user_agent) VALUES ('legacy', '198.51.100.10', 'legacy-agent');",
                       NULL, NULL, NULL) == SQLITE_OK);
    CHECK(sqlite3_close(legacy_db) == SQLITE_OK);

    CHECK(chdir(directory) == 0);
    CHECK(init_db("data/shortener.db", "test-analytics-hmac-key") == 0);

    char target[2049];
    CHECK(get_link_with_password("legacy", "old-password", target, sizeof(target)) == 0);
    CHECK(strcmp(target, "https://example.test/legacy") == 0);

    sqlite3_stmt *statement = NULL;
    CHECK(sqlite3_prepare_v2(db, "SELECT password FROM links WHERE slug = 'legacy';", -1, &statement, NULL) == SQLITE_OK);
    CHECK(sqlite3_step(statement) == SQLITE_ROW);
    const unsigned char *stored_password = sqlite3_column_text(statement, 0);
    CHECK(stored_password != NULL);
    CHECK(strncmp((const char *)stored_password, "pbkdf2-sha256$600000$", 20U) == 0);
    sqlite3_finalize(statement);

    CHECK(sqlite3_prepare_v2(db, "SELECT ip, user_agent FROM visits WHERE slug = 'legacy';", -1, &statement, NULL) == SQLITE_OK);
    CHECK(sqlite3_step(statement) == SQLITE_ROW);
    const unsigned char *stored_ip = sqlite3_column_text(statement, 0);
    CHECK(stored_ip != NULL);
    CHECK(strncmp((const char *)stored_ip, "hmac-sha256:v1:", 15U) == 0);
    CHECK(sqlite3_column_type(statement, 1) == SQLITE_NULL);
    sqlite3_finalize(statement);

    CHECK(insert_link("new-link", "https://example.test/new", 0, "new-password") == 0);
    int requires_password = 0;
    CHECK(get_link("new-link", target, sizeof(target), &requires_password) == 0);
    CHECK(requires_password == 1);
    CHECK(get_link_with_password("new-link", "new-password", target, sizeof(target)) == 0);
    CHECK(get_link_with_password("new-link", "wrong-password", target, sizeof(target)) == -4);

    // Ensure inspect JSON redacts target_url when link is password-protected
    char *inspect_pwd = get_link_inspect_json("new-link");
    CHECK(inspect_pwd != NULL);
    if (inspect_pwd) {
        CHECK(strstr(inspect_pwd, "\"has_password\":true") != NULL);
        CHECK(strstr(inspect_pwd, "https://example.test/new") == NULL);
        free(inspect_pwd);
    }

    CHECK(record_visit_ex("new-link", "127.0.0.1", "desktop", "github.com") == 0);
    CHECK(record_visit_ex("new-link", "127.0.0.1", "mobile", "direct") == 0);
    int total = 0;
    int unique = 0;
    CHECK(get_stats("new-link", &total, &unique) == 0);
    CHECK(total == 2 && unique == 1);

    // Test 1-click self-destruct link (max_clicks = 1) and scheduled link
    struct link_options burn_opts = { 0, 1, 0, NULL, "One-time secret", "burn,security", NULL, NULL };
    CHECK(insert_link_ex("burn-once", "https://example.test/secret-doc", &burn_opts) == 0);
    CHECK(get_link("burn-once", target, sizeof(target), &requires_password) == 0);
    CHECK(record_visit_ex("burn-once", "203.0.113.5", "desktop", "direct") == 0);
    CHECK(get_link("burn-once", target, sizeof(target), &requires_password) == -3);

    struct link_options sched_opts = { 0, 0, 24, NULL, "Future launch", "launch", NULL, NULL };
    CHECK(insert_link_ex("future-link", "https://example.test/launch", &sched_opts) == 0);
    CHECK(get_link("future-link", target, sizeof(target), &requires_password) == -5);

    // Test smart device routing (mobile_url) & idempotency key lookup
    struct link_options smart_opts = {
        0, 0, 0, NULL, "Smart App Link", "launch,mobile",
        "https://m.example.test/app", "idem-key-2026-001"
    };
    CHECK(insert_link_ex("smart-app", "https://example.test/desktop", &smart_opts) == 0);
    char idem_slug[64];
    CHECK(find_slug_by_idempotency_key("idem-key-2026-001", idem_slug, sizeof(idem_slug)) == 0);
    CHECK(strcmp(idem_slug, "smart-app") == 0);
    CHECK(find_slug_by_idempotency_key("non-existent-idem", idem_slug, sizeof(idem_slug)) == -1);
    CHECK(get_link_for_device("smart-app", "desktop", target, sizeof(target), &requires_password) == 0);
    CHECK(strcmp(target, "https://example.test/desktop") == 0);
    CHECK(get_link_for_device("smart-app", "mobile", target, sizeof(target), &requires_password) == 0);
    CHECK(strcmp(target, "https://m.example.test/app") == 0);

    // Test Link-in-Bio collection JSON, Prometheus metrics, and System diagnostics JSON
    char *bio_json = get_bio_collection_json("mobile");
    CHECK(bio_json != NULL);
    if (bio_json) {
        CHECK(strstr(bio_json, "smart-app") != NULL);
        free(bio_json);
    }

    char *prom = get_prometheus_metrics(120L);
    CHECK(prom != NULL);
    if (prom) {
        CHECK(strstr(prom, "shortener_links_total") != NULL);
        CHECK(strstr(prom, "shortener_uptime_seconds") != NULL);
        free(prom);
    }

    char *sys_json = get_system_diagnostics_json(120L);
    CHECK(sys_json != NULL);
    if (sys_json) {
        CHECK(strstr(sys_json, "\"sqlite_version\"") != NULL);
        CHECK(strstr(sys_json, "\"hardening\"") != NULL);
        free(sys_json);
    }

    // Test link editing & disable/enable toggle
    CHECK(update_link("burn-once", "https://example.test/updated-doc", "Updated title",
                      "work,updated", 48, 5, 0, NULL) == 0);
    CHECK(get_link("burn-once", target, sizeof(target), &requires_password) == -6);
    CHECK(update_link("burn-once", "https://example.test/updated-doc", "Updated title",
                      "work,updated", 48, 5, 1, NULL) == 0);
    CHECK(get_link("burn-once", target, sizeof(target), &requires_password) == 0);
    CHECK(strcmp(target, "https://example.test/updated-doc") == 0);

    // Test detailed stats, admin query filtering, and CSV export
    char *detailed = get_detailed_stats_json("burn-once");
    CHECK(detailed != NULL);
    if (detailed) {
        CHECK(strstr(detailed, "\"remaining_clicks\":4") != NULL);
        free(detailed);
    }

    char *filtered = query_admin_links_json("Updated", "work", "active", 50, 0);
    CHECK(filtered != NULL);
    if (filtered) {
        CHECK(strstr(filtered, "burn-once") != NULL);
        free(filtered);
    }

    char *csv = export_links_csv();
    CHECK(csv != NULL);
    if (csv) {
        CHECK(strstr(csv, "\"burn-once\"") != NULL);
        free(csv);
    }

    // Test scoped creator tokens & security audit logs
    char raw_token[64];
    char token_actor[64];
    CHECK(create_api_token("CI Pipeline", 30, raw_token, sizeof(raw_token)) == 0);
    CHECK(verify_creator_token(raw_token, token_actor, sizeof(token_actor)) == 1);
    CHECK(strcmp(token_actor, "token:CI Pipeline") == 0);
    CHECK(verify_creator_token("csh_invalidtoken000000000000000000000000", NULL, 0) == 0);

    char *tokens_json = list_api_tokens_json();
    CHECK(tokens_json != NULL);
    if (tokens_json) {
        CHECK(strstr(tokens_json, "CI Pipeline") != NULL);
        free(tokens_json);
    }
    CHECK(revoke_api_token(1) == 0);
    CHECK(verify_creator_token(raw_token, NULL, 0) == 0);

    CHECK(record_audit_event("link.create", "admin", "burn-once", "127.0.0.1", "Created test link") == 0);
    char *audit_json = get_audit_logs_json(20);
    CHECK(audit_json != NULL);
    if (audit_json) {
        CHECK(strstr(audit_json, "link.create") != NULL);
        free(audit_json);
    }

    CHECK(check_rate_limit("test-rate-key", 2) == 0);
    CHECK(check_rate_limit("test-rate-key", 2) == 0);
    CHECK(check_rate_limit("test-rate-key", 2) != 0);
    CHECK(delete_link("new-link") == 0);
    CHECK(get_link("new-link", target, sizeof(target), &requires_password) == -1);

    close_db();
    CHECK(chdir(old_directory) == 0);
    cleanup_test_files(directory);

    if (failures != 0) return 1;
    puts("test_db: ok");
    return 0;
}
