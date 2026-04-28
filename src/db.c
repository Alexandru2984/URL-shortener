#include "db.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

sqlite3 *db = NULL;

int init_db(const char *db_path) {
    if (sqlite3_open(db_path, &db) != SQLITE_OK) {
        log_error("Cannot open database: %s", sqlite3_errmsg(db));
        return -1;
    }

    const char *sql_links = "CREATE TABLE IF NOT EXISTS links ("
                            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                            "slug TEXT UNIQUE NOT NULL,"
                            "url TEXT NOT NULL,"
                            "created_at DATETIME DEFAULT CURRENT_TIMESTAMP);";
                            
    const char *sql_visits = "CREATE TABLE IF NOT EXISTS visits ("
                             "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                             "slug TEXT NOT NULL,"
                             "ip TEXT,"
                             "user_agent TEXT,"
                             "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";
                             
    const char *sql_rate = "CREATE TABLE IF NOT EXISTS rate_limit ("
                           "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                           "ip TEXT NOT NULL,"
                           "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";

    char *err_msg = NULL;
    if (sqlite3_exec(db, sql_links, 0, 0, &err_msg) != SQLITE_OK) {
        log_error("Failed to create links table: %s", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }
    if (sqlite3_exec(db, sql_visits, 0, 0, &err_msg) != SQLITE_OK) {
        log_error("Failed to create visits table: %s", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }
    if (sqlite3_exec(db, sql_rate, 0, 0, &err_msg) != SQLITE_OK) {
        log_error("Failed to create rate_limit table: %s", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }

    return 0;
}

int insert_link(const char *slug, const char *url) {
    const char *sql = "INSERT INTO links (slug, url) VALUES (?, ?);";
    sqlite3_stmt *res;
    if (sqlite3_prepare_v2(db, sql, -1, &res, 0) != SQLITE_OK) return -1;
    
    sqlite3_bind_text(res, 1, slug, -1, SQLITE_STATIC);
    sqlite3_bind_text(res, 2, url, -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(res);
    sqlite3_finalize(res);
    
    if (rc != SQLITE_DONE) return -1;
    return 0;
}

int get_link(const char *slug, char *url_out, int max_len) {
    const char *sql = "SELECT url FROM links WHERE slug = ?;";
    sqlite3_stmt *res;
    if (sqlite3_prepare_v2(db, sql, -1, &res, 0) != SQLITE_OK) return -1;
    
    sqlite3_bind_text(res, 1, slug, -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(res);
    int found = 0;
    if (rc == SQLITE_ROW) {
        const unsigned char *url = sqlite3_column_text(res, 0);
        if (url) {
            strncpy(url_out, (const char *)url, max_len - 1);
            url_out[max_len - 1] = '\0';
            found = 1;
        }
    }
    sqlite3_finalize(res);
    return found ? 0 : -1;
}

int record_visit(const char *slug, const char *ip, const char *user_agent) {
    const char *sql = "INSERT INTO visits (slug, ip, user_agent) VALUES (?, ?, ?);";
    sqlite3_stmt *res;
    if (sqlite3_prepare_v2(db, sql, -1, &res, 0) != SQLITE_OK) return -1;
    
    sqlite3_bind_text(res, 1, slug, -1, SQLITE_STATIC);
    sqlite3_bind_text(res, 2, ip, -1, SQLITE_STATIC);
    sqlite3_bind_text(res, 3, user_agent, -1, SQLITE_STATIC);
    
    sqlite3_step(res);
    sqlite3_finalize(res);
    return 0;
}

int get_stats(const char *slug, int *total_visits, int *unique_visits) {
    const char *sql_total = "SELECT COUNT(*) FROM visits WHERE slug = ?;";
    sqlite3_stmt *res;
    if (sqlite3_prepare_v2(db, sql_total, -1, &res, 0) == SQLITE_OK) {
        sqlite3_bind_text(res, 1, slug, -1, SQLITE_STATIC);
        if (sqlite3_step(res) == SQLITE_ROW) {
            *total_visits = sqlite3_column_int(res, 0);
        }
        sqlite3_finalize(res);
    } else {
        *total_visits = 0;
    }

    const char *sql_unique = "SELECT COUNT(DISTINCT ip) FROM visits WHERE slug = ?;";
    if (sqlite3_prepare_v2(db, sql_unique, -1, &res, 0) == SQLITE_OK) {
        sqlite3_bind_text(res, 1, slug, -1, SQLITE_STATIC);
        if (sqlite3_step(res) == SQLITE_ROW) {
            *unique_visits = sqlite3_column_int(res, 0);
        }
        sqlite3_finalize(res);
    } else {
        *unique_visits = 0;
    }

    return 0;
}

int check_rate_limit(const char *ip, int max_requests_per_min) {
    const char *sql_insert = "INSERT INTO rate_limit (ip) VALUES (?);";
    sqlite3_stmt *res_insert;
    if (sqlite3_prepare_v2(db, sql_insert, -1, &res_insert, 0) == SQLITE_OK) {
        sqlite3_bind_text(res_insert, 1, ip, -1, SQLITE_STATIC);
        sqlite3_step(res_insert);
        sqlite3_finalize(res_insert);
    }
    
    const char *sql_clean = "DELETE FROM rate_limit WHERE timestamp <= datetime('now', '-1 minute');";
    sqlite3_exec(db, sql_clean, 0, 0, NULL);
    
    const char *sql_count = "SELECT COUNT(*) FROM rate_limit WHERE ip = ? AND timestamp > datetime('now', '-1 minute');";
    sqlite3_stmt *res_count;
    int count = 0;
    if (sqlite3_prepare_v2(db, sql_count, -1, &res_count, 0) == SQLITE_OK) {
        sqlite3_bind_text(res_count, 1, ip, -1, SQLITE_STATIC);
        if (sqlite3_step(res_count) == SQLITE_ROW) {
            count = sqlite3_column_int(res_count, 0);
        }
        sqlite3_finalize(res_count);
    }
    
    if (count > max_requests_per_min) {
        return -1; // Rate limit exceeded
    }
    return 0; // OK
}
