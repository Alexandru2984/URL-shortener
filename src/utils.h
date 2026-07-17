#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

int generate_random_slug(char *slug_out, size_t len);
void slugify(const char *input, char *output, int max_len);
int is_valid_http_url(const char *url, size_t max_len);
int is_valid_slug(const char *slug, size_t max_len);
void log_message(const char *format, ...);
void log_error(const char *format, ...);

// Password hashing (PBKDF2-HMAC-SHA-256). hash_out must be at least 128 bytes.
int hash_password(const char *password, char *hash_out, size_t hash_out_len);
int verify_password(const char *password, const char *stored_hash);
int password_needs_rehash(const char *stored_hash);

#endif
