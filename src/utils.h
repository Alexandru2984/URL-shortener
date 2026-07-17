#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

void generate_random_slug(char *slug_out, int len);
void slugify(const char *input, char *output, int max_len);
int is_valid_http_url(const char *url, size_t max_len);
int is_valid_slug(const char *slug, size_t max_len);
void log_message(const char *format, ...);
void log_error(const char *format, ...);
int is_port_in_use(int port);
int find_available_port(int start_port);

// Password hashing (SHA-256 + random salt)
// hash_out must be at least 97 bytes (32 salt hex + ':' + 64 hash hex)
void hash_password(const char *password, char *hash_out, size_t hash_out_len);
int verify_password(const char *password, const char *stored_hash);

#endif
