#ifndef DB_H
#define DB_H

#include <stddef.h>
#include <sqlite3.h>

extern sqlite3 *db;

struct link_options {
    int ttl_hours;
    int max_clicks;
    int activates_in_hours;
    const char *password;
    const char *title;
    const char *tags;
};

int init_db(const char *db_path, const char *analytics_hmac_key);
int insert_link(const char *slug, const char *url, int ttl_hours, const char *password);
int insert_link_ex(const char *slug, const char *url, const struct link_options *opts);
int update_link(const char *slug, const char *new_url, const char *title,
                const char *tags, int ttl_hours, int max_clicks,
                int is_active, const char *new_password);

int get_link(const char *slug, char *url_out, size_t url_out_len, int *requires_pwd);
int get_link_with_password(const char *slug, const char *password, char *url_out, size_t url_out_len);

int record_visit(const char *slug, const char *ip);
int record_visit_ex(const char *slug, const char *ip,
                    const char *device_type, const char *referrer_domain);

int get_stats(const char *slug, int *total_visits, int *unique_visits);
char *get_detailed_stats_json(const char *slug);
char *get_link_inspect_json(const char *slug);

int check_rate_limit(const char *ip, int max_requests_per_min);
int delete_link(const char *slug);
int cleanup_expired_links(void);
int cleanup_old_analytics(int retention_days);

// Scoped creator tokens
int create_api_token(const char *name, int ttl_days,
                     char *raw_token_out, size_t raw_token_len);
int verify_creator_token(const char *raw_token, char *token_name_out, size_t name_len);
char *list_api_tokens_json(void);
int revoke_api_token(int token_id);

// Security audit logging
int record_audit_event(const char *event_type, const char *actor,
                       const char *target, const char *ip, const char *details);
char *get_audit_logs_json(int limit);

// Admin: query links as JSON string or export as CSV (caller must free)
char *get_all_links_json(void);
char *query_admin_links_json(const char *search, const char *tag,
                             const char *status_filter, int limit, int offset);
char *export_links_csv(void);

void close_db(void);

#endif
