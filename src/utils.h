#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

int generate_random_slug(char *slug_out, size_t len);
int generate_api_token(char *token_out, size_t token_out_len,
                       char *prefix_out, size_t prefix_out_len);
int hash_api_token(const char *token, char *hash_out, size_t hash_out_len);

void slugify(const char *input, char *output, int max_len);
int is_valid_http_url(const char *url, size_t max_len);
int is_safe_redirect_url(const char *url, const char *base_url, size_t max_len);
int is_valid_slug(const char *slug, size_t max_len);
int is_valid_tag_list(const char *tags, size_t max_len);

void classify_user_agent(const char *user_agent, char *device_out, size_t device_out_len);
void classify_referrer(const char *referer, const char *base_url,
                       char *domain_out, size_t domain_out_len);
void sanitize_csv_field(const char *input, char *output, size_t output_len);
void html_escape(const char *input, char *output, size_t output_len);

void log_message(const char *format, ...);
void log_error(const char *format, ...);

// Password hashing (PBKDF2-HMAC-SHA-256). hash_out must be at least 128 bytes.
int hash_password(const char *password, char *hash_out, size_t hash_out_len);
int verify_password(const char *password, const char *stored_hash);
int password_needs_rehash(const char *stored_hash);

#endif
