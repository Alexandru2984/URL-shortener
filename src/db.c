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
                            "expires_at DATETIME,"
                            "password TEXT,"
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
    
    // Add columns if they don't exist (ignore errors)
    sqlite3_exec(db, "ALTER TABLE links ADD COLUMN expires_at DATETIME;", 0, 0, NULL);
    sqlite3_exec(db, "ALTER TABLE links ADD COLUMN password TEXT;", 0, 0, NULL);

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

int insert_link(const char *slug, const char *url, int ttl_hours, const char *password) {
    const char *sql;
    if (ttl_hours > 0) {
        sql = "INSERT INTO links (slug, url, expires_at, password) VALUES (?, ?, datetime('now', '+' || ? || ' hours'), ?);";
    } else {
        sql = "INSERT INTO links (slug, url, password) VALUES (?, ?, ?);";
    }
    
    sqlite3_stmt *res;
    if (sqlite3_prepare_v2(db, sql, -1, &res, 0) != SQLITE_OK) return -1;
    
    sqlite3_bind_text(res, 1, slug, -1, SQLITE_STATIC);
    sqlite3_bind_text(res, 2, url, -1, SQLITE_STATIC);
    
    if (ttl_hours > 0) {
        sqlite3_bind_int(res, 3, ttl_hours);
        sqlite3_bind_text(res, 4, password && strlen(password) > 0 ? password : NULL, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_text(res, 3, password && strlen(password) > 0 ? password : NULL, -1, SQLITE_STATIC);
    }
    
    int rc = sqlite3_step(res);
    sqlite3_finalize(res);
    
    if (rc != SQLITE_DONE) return -1;
    return 0;
}

int get_link(const char *slug, char *url_out, int max_len, int *requires_pwd) {
    *requires_pwd = 0;
    
    // Check expiration and get data
    const char *sql = "SELECT url, password, expires_at FROM links WHERE slug = ?;";
    sqlite3_stmt *res;
    if (sqlite3_prepare_v2(db, sql, -1, &res, 0) != SQLITE_OK) return -1;
    
    sqlite3_bind_text(res, 1, slug, -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(res);
    if (rc == SQLITE_ROW) {
        const unsigned char *url = sqlite3_column_text(res, 0);
        const unsigned char *password = sqlite3_column_text(res, 1);
        const unsigned char *expires_at = sqlite3_column_text(res, 2);
        
        if (expires_at) {
            // Check if expired
            const char *sql_check = "SELECT 1 WHERE datetime('now') > ?;";
            sqlite3_stmt *check_res;
            if (sqlite3_prepare_v2(db, sql_check, -1, &check_res, 0) == SQLITE_OK) {
                sqlite3_bind_text(check_res, 1, (const char *)expires_at, -1, SQLITE_STATIC);
                if (sqlite3_step(check_res) == SQLITE_ROW) {
                    sqlite3_finalize(check_res);
                    sqlite3_finalize(res);
                    return -2; // Expired
                }
                sqlite3_finalize(check_res);
            }
        }
        
        if (password && strlen((const char *)password) > 0) {
            *requires_pwd = 1;
            sqlite3_finalize(res);
            return 0; // Requires password
        }
        
        if (url) {
            strncpy(url_out, (const char *)url, max_len - 1);
            url_out[max_len - 1] = '\0';
            sqlite3_finalize(res);
            return 0;
        }
    }
    
    sqlite3_finalize(res);
    return -1; // Not found
}

int get_link_with_password(const char *slug, const char *password_in, char *url_out, int max_len) {
    const char *sql = "SELECT url, password, expires_at FROM links WHERE slug = ?;";
    sqlite3_stmt *res;
    if (sqlite3_prepare_v2(db, sql, -1, &res, 0) != SQLITE_OK) return -1;
    
    sqlite3_bind_text(res, 1, slug, -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(res);
    if (rc == SQLITE_ROW) {
        const unsigned char *url = sqlite3_column_text(res, 0);
        const unsigned char *password = sqlite3_column_text(res, 1);
        const unsigned char *expires_at = sqlite3_column_text(res, 2);
        
        if (expires_at) {
            // Check if expired
            const char *sql_check = "SELECT 1 WHERE datetime('now') > ?;";
            sqlite3_stmt *check_res;
            if (sqlite3_prepare_v2(db, sql_check, -1, &check_res, 0) == SQLITE_OK) {
                sqlite3_bind_text(check_res, 1, (const char *)expires_at, -1, SQLITE_STATIC);
                if (sqlite3_step(check_res) == SQLITE_ROW) {
                    sqlite3_finalize(check_res);
                    sqlite3_finalize(res);
                    return -2; // Expired
                }
                sqlite3_finalize(check_res);
            }
        }
        
        if (password && password_in && strcmp((const char *)password, password_in) == 0) {
            if (url) {
                strncpy(url_out, (const char *)url, max_len - 1);
                url_out[max_len - 1] = '\0';
                sqlite3_finalize(res);
                return 0;
            }
        } else {
            sqlite3_finalize(res);
            return -4; // Wrong password
        }
    }
    
    sqlite3_finalize(res);
    return -1; // Not found
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
