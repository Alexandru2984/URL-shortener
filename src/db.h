#ifndef DB_H
#define DB_H

#include <stddef.h>
#include <sqlite3.h>

extern sqlite3 *db;

int init_db(const char *db_path, const char *analytics_hmac_key);
int insert_link(const char *slug, const char *url, int ttl_hours, const char *password);
int get_link(const char *slug, char *url_out, size_t url_out_len, int *requires_pwd);
int get_link_with_password(const char *slug, const char *password, char *url_out, size_t url_out_len);
int record_visit(const char *slug, const char *ip);
int get_stats(const char *slug, int *total_visits, int *unique_visits);
int check_rate_limit(const char *ip, int max_requests_per_min);
int delete_link(const char *slug);
int cleanup_expired_links(void);

// Admin: get all links as JSON string (caller must free)
char *get_all_links_json(void);

void close_db(void);

#endif
