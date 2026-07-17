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
                       "INSERT INTO links (slug, url, password) VALUES ('legacy', 'https://example.test/legacy', 'old-password');",
                       NULL, NULL, NULL) == SQLITE_OK);
    CHECK(sqlite3_close(legacy_db) == SQLITE_OK);

    CHECK(chdir(directory) == 0);
    CHECK(init_db("data/shortener.db") == 0);

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

    CHECK(insert_link("new-link", "https://example.test/new", 0, "new-password") == 0);
    int requires_password = 0;
    CHECK(get_link("new-link", target, sizeof(target), &requires_password) == 0);
    CHECK(requires_password == 1);
    CHECK(get_link_with_password("new-link", "new-password", target, sizeof(target)) == 0);
    CHECK(get_link_with_password("new-link", "wrong-password", target, sizeof(target)) == -4);

    CHECK(record_visit("new-link", "127.0.0.1", "test-agent") == 0);
    CHECK(record_visit("new-link", "127.0.0.1", "test-agent") == 0);
    int total = 0;
    int unique = 0;
    CHECK(get_stats("new-link", &total, &unique) == 0);
    CHECK(total == 2 && unique == 1);
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
