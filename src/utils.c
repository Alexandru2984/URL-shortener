#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <pthread.h>
#include <openssl/evp.h>

void slugify(const char *input, char *output, int max_len) {
    int i = 0, j = 0;
    while (input[i] && j < max_len - 1) {
        char c = input[i];
        if (isalnum((unsigned char)c)) {
            output[j++] = tolower((unsigned char)c);
        } else if (c == ' ' || c == '-' || c == '_') {
            if (j > 0 && output[j-1] != '-') {
                output[j++] = '-';
            }
        }
        i++;
    }
    if (j > 0 && output[j-1] == '-') j--;
    output[j] = '\0';
}

void generate_random_slug(char *slug_out, int len) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const int charset_len = (int)(sizeof(charset) - 1);
    unsigned char rand_bytes[64]; // Max slug length supported
    if (len > (int)sizeof(rand_bytes)) len = (int)sizeof(rand_bytes);

    FILE *urandom = fopen("/dev/urandom", "rb");
    if (urandom) {
        size_t read = fread(rand_bytes, 1, len, urandom);
        fclose(urandom);
        if (read != (size_t)len) {
            // Fallback: should not happen on Linux
            log_error("Failed to read from /dev/urandom");
        }
    } else {
        // Last resort fallback (should never happen on Linux)
        log_error("Cannot open /dev/urandom, falling back to rand()");
        for (int i = 0; i < len; i++) {
            rand_bytes[i] = (unsigned char)(rand() % 256);
        }
    }

    for (int i = 0; i < len; i++) {
        slug_out[i] = charset[rand_bytes[i] % charset_len];
    }
    slug_out[len] = '\0';
}
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

static void log_to_file(const char *filename, const char *level, const char *format, va_list args) {
    pthread_mutex_lock(&log_mutex);

    FILE *f = fopen(filename, "a");
    if (!f) {
        pthread_mutex_unlock(&log_mutex);
        return;
    }
    
    time_t now;
    time(&now);
    struct tm tm_buf;
    struct tm *tm_info = localtime_r(&now, &tm_buf);
    char time_buf[26];
    strftime(time_buf, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    
    fprintf(f, "[%s] [%s] ", time_buf, level);
    vfprintf(f, format, args);
    fprintf(f, "\n");
    fclose(f);

    pthread_mutex_unlock(&log_mutex);
}

void log_message(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_to_file("logs/access.log", "INFO", format, args);
    va_end(args);
    
    // Also print to stdout
    va_start(args, format);
    printf("[INFO] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void log_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_to_file("logs/error.log", "ERROR", format, args);
    va_end(args);
    
    // Also print to stderr
    va_start(args, format);
    fprintf(stderr, "[ERROR] ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

int is_port_in_use(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return 1; // Assume in use if we can't create socket
    
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    
    int result = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    close(sockfd);
    
    if (result < 0) return 1; // Port in use
    return 0; // Port available
}

int find_available_port(int start_port) {
    int port = start_port;
    while (is_port_in_use(port) && port < 65535) {
        port++;
    }
    return port;
}

// --- Password hashing (SHA-256 with random salt) ---

static void bytes_to_hex(const unsigned char *bytes, int len, char *hex_out) {
    for (int i = 0; i < len; i++) {
        sprintf(hex_out + (i * 2), "%02x", bytes[i]);
    }
    hex_out[len * 2] = '\0';
}

static int hex_to_bytes(const char *hex, unsigned char *bytes_out, int max_bytes) {
    int hex_len = (int)strlen(hex);
    int byte_len = hex_len / 2;
    if (byte_len > max_bytes) byte_len = max_bytes;
    for (int i = 0; i < byte_len; i++) {
        unsigned int val;
        if (sscanf(hex + (i * 2), "%02x", &val) != 1) return -1;
        bytes_out[i] = (unsigned char)val;
    }
    return byte_len;
}

void hash_password(const char *password, char *hash_out, size_t hash_out_len) {
    if (!password || hash_out_len < 97) {
        if (hash_out && hash_out_len > 0) hash_out[0] = '\0';
        return;
    }

    // Generate 16-byte random salt from /dev/urandom
    unsigned char salt[16];
    FILE *urandom = fopen("/dev/urandom", "rb");
    if (urandom) {
        if (fread(salt, 1, sizeof(salt), urandom) != sizeof(salt)) {
            log_error("Short read from /dev/urandom for password salt");
        }
        fclose(urandom);
    } else {
        // Fallback (should not happen on Linux)
        for (int i = 0; i < 16; i++) salt[i] = (unsigned char)(rand() % 256);
    }

    // SHA-256(salt + password)
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (ctx) {
        EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
        EVP_DigestUpdate(ctx, salt, sizeof(salt));
        EVP_DigestUpdate(ctx, password, strlen(password));
        EVP_DigestFinal_ex(ctx, hash, &hash_len);
        EVP_MD_CTX_free(ctx);
    }

    // Output format: hex(salt):hex(hash) → 32 + 1 + 64 = 97 chars
    char salt_hex[33];
    char hash_hex[65];
    bytes_to_hex(salt, 16, salt_hex);
    bytes_to_hex(hash, (int)hash_len, hash_hex);
    snprintf(hash_out, hash_out_len, "%s:%s", salt_hex, hash_hex);
}

int verify_password(const char *password, const char *stored_hash) {
    if (!password || !stored_hash) return 0;

    // Parse "salt_hex:hash_hex"
    const char *colon = strchr(stored_hash, ':');
    if (!colon) {
        // Legacy plain-text comparison (for existing DB entries)
        return strcmp(password, stored_hash) == 0;
    }

    int salt_hex_len = (int)(colon - stored_hash);
    if (salt_hex_len != 32) return 0; // Invalid format

    char salt_hex[33];
    memcpy(salt_hex, stored_hash, 32);
    salt_hex[32] = '\0';

    unsigned char salt[16];
    if (hex_to_bytes(salt_hex, salt, 16) != 16) return 0;

    // Recompute SHA-256(salt + password)
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (ctx) {
        EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
        EVP_DigestUpdate(ctx, salt, sizeof(salt));
        EVP_DigestUpdate(ctx, password, strlen(password));
        EVP_DigestFinal_ex(ctx, hash, &hash_len);
        EVP_MD_CTX_free(ctx);
    }

    char computed_hex[65];
    bytes_to_hex(hash, (int)hash_len, computed_hex);

    const char *expected_hex = colon + 1;
    return strcmp(computed_hex, expected_hex) == 0;
}
