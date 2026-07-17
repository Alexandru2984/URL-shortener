#include "utils.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <pthread.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <strings.h>

void slugify(const char *input, char *output, int max_len) {
    if (!input || !output || max_len < 1) return;

    int i = 0, j = 0;
    while (input[i] && j < max_len - 1) {
        char c = input[i];
        if (isalnum((unsigned char)c)) {
            output[j++] = (char)tolower((unsigned char)c);
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

int is_valid_slug(const char *slug, size_t max_len) {
    if (!slug || max_len == 0) return 0;

    size_t len = strnlen(slug, max_len + 1);
    if (len == 0 || len > max_len || slug[0] == '-' || slug[len - 1] == '-') return 0;

    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)slug[i];
        if (!isalnum(c) && c != '-') return 0;
    }
    return 1;
}

int is_valid_http_url(const char *url, size_t max_len) {
    if (!url || max_len == 0) return 0;

    size_t len = strnlen(url, max_len + 1);
    if (len == 0 || len > max_len) return 0;

    size_t scheme_len;
    if (strncasecmp(url, "https://", 8) == 0) {
        scheme_len = 8;
    } else if (strncasecmp(url, "http://", 7) == 0) {
        scheme_len = 7;
    } else {
        return 0;
    }

    size_t authority_end = scheme_len;
    while (authority_end < len && url[authority_end] != '/' &&
           url[authority_end] != '?' && url[authority_end] != '#') {
        authority_end++;
    }
    if (authority_end == scheme_len) return 0;

    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)url[i];
        if (c <= 0x20 || c > 0x7e || c == '\\' ||
            c == '"' || c == '\'' || c == '<' || c == '>') {
            return 0;
        }
    }

    // Credentials in a redirect target are both a phishing risk and rarely useful.
    for (size_t i = scheme_len; i < authority_end; i++) {
        if (url[i] == '@') return 0;
    }

    return 1;
}

int generate_random_slug(char *slug_out, size_t len) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const size_t charset_len = sizeof(charset) - 1U;
    const unsigned int unbiased_limit = 256U - (256U % (unsigned int)charset_len);
    unsigned char random_bytes[64];
    size_t written = 0;

    if (!slug_out || len == 0 || len > 64U) return -1;

    while (written < len) {
        if (RAND_bytes(random_bytes, (int)sizeof(random_bytes)) != 1) {
            slug_out[0] = '\0';
            return -1;
        }
        for (size_t i = 0; i < sizeof(random_bytes) && written < len; i++) {
            if ((unsigned int)random_bytes[i] >= unbiased_limit) continue;
            slug_out[written++] = charset[random_bytes[i] % charset_len];
        }
    }
    slug_out[len] = '\0';
    return 0;
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
    const struct tm *tm_info = localtime_r(&now, &tm_buf);
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

// --- Password hashing (PBKDF2-HMAC-SHA-256) ---

#define PASSWORD_SALT_LEN 16U
#define PASSWORD_DIGEST_LEN 32U
#define PBKDF2_ITERATIONS 600000U
#define PASSWORD_HASH_MAX_LEN 128U
#define PBKDF2_PREFIX "pbkdf2-sha256"

static void bytes_to_hex(const unsigned char *bytes, size_t len, char *hex_out) {
    static const char digits[] = "0123456789abcdef";
    for (size_t i = 0; i < len; i++) {
        hex_out[i * 2U] = digits[bytes[i] >> 4U];
        hex_out[i * 2U + 1U] = digits[bytes[i] & 0x0fU];
    }
    hex_out[len * 2U] = '\0';
}

static int hex_value(unsigned char value) {
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    if (value >= 'A' && value <= 'F') return value - 'A' + 10;
    return -1;
}

static int hex_to_bytes(const char *hex, size_t hex_len, unsigned char *bytes_out,
                        size_t bytes_out_len) {
    if (!hex || !bytes_out || hex_len != bytes_out_len * 2U) return -1;
    for (size_t i = 0; i < bytes_out_len; i++) {
        int high = hex_value((unsigned char)hex[i * 2U]);
        int low = hex_value((unsigned char)hex[i * 2U + 1U]);
        if (high < 0 || low < 0) return -1;
        bytes_out[i] = (unsigned char)((high << 4) | low);
    }
    return 0;
}

static int derive_pbkdf2(const char *password, const unsigned char *salt,
                         unsigned int iterations, unsigned char *digest) {
    size_t password_len = strlen(password);
    if (password_len > (size_t)INT_MAX || iterations == 0) return -1;
    return PKCS5_PBKDF2_HMAC(password, (int)password_len, salt, (int)PASSWORD_SALT_LEN,
                             (int)iterations, EVP_sha256(), (int)PASSWORD_DIGEST_LEN,
                             digest) == 1 ? 0 : -1;
}

static int secure_string_equal(const char *left, const char *right) {
    size_t left_len = strlen(left);
    size_t right_len = strlen(right);
    if (left_len != right_len) return 0;
    return CRYPTO_memcmp(left, right, left_len) == 0;
}

static int parse_pbkdf2_hash(const char *stored_hash, unsigned int *iterations_out,
                             unsigned char *salt_out, unsigned char *digest_out) {
    const size_t prefix_len = sizeof(PBKDF2_PREFIX) - 1U;
    size_t stored_len = strnlen(stored_hash, PASSWORD_HASH_MAX_LEN + 1U);
    if (stored_len < prefix_len + 2U || stored_len > PASSWORD_HASH_MAX_LEN ||
        strncmp(stored_hash, PBKDF2_PREFIX, prefix_len) != 0 ||
        stored_hash[prefix_len] != '$') {
        return -1;
    }

    const char *iterations_start = stored_hash + prefix_len + 1U;
    char *iterations_end = NULL;
    errno = 0;
    unsigned long parsed_iterations = strtoul(iterations_start, &iterations_end, 10);
    if (errno != 0 || iterations_end == iterations_start || *iterations_end != '$' ||
        parsed_iterations == 0 || parsed_iterations > UINT_MAX) {
        return -1;
    }

    const char *salt_hex = iterations_end + 1U;
    const char *digest_separator = strchr(salt_hex, '$');
    if (!digest_separator || (size_t)(digest_separator - salt_hex) != PASSWORD_SALT_LEN * 2U ||
        strlen(digest_separator + 1U) != PASSWORD_DIGEST_LEN * 2U ||
        hex_to_bytes(salt_hex, PASSWORD_SALT_LEN * 2U, salt_out, PASSWORD_SALT_LEN) != 0 ||
        hex_to_bytes(digest_separator + 1U, PASSWORD_DIGEST_LEN * 2U, digest_out,
                     PASSWORD_DIGEST_LEN) != 0) {
        return -1;
    }

    *iterations_out = (unsigned int)parsed_iterations;
    return 0;
}

int hash_password(const char *password, char *hash_out, size_t hash_out_len) {
    const size_t required_len = (sizeof(PBKDF2_PREFIX) - 1U) + 1U + 6U + 1U +
                                (PASSWORD_SALT_LEN * 2U) + 1U + (PASSWORD_DIGEST_LEN * 2U) + 1U;
    if (!password || !hash_out || hash_out_len < required_len || strlen(password) > (size_t)INT_MAX) {
        if (hash_out && hash_out_len > 0) hash_out[0] = '\0';
        return -1;
    }

    unsigned char salt[PASSWORD_SALT_LEN];
    unsigned char digest[PASSWORD_DIGEST_LEN];
    if (RAND_bytes(salt, (int)sizeof(salt)) != 1 ||
        derive_pbkdf2(password, salt, PBKDF2_ITERATIONS, digest) != 0) {
        hash_out[0] = '\0';
        return -1;
    }

    char salt_hex[PASSWORD_SALT_LEN * 2U + 1U];
    char digest_hex[PASSWORD_DIGEST_LEN * 2U + 1U];
    bytes_to_hex(salt, sizeof(salt), salt_hex);
    bytes_to_hex(digest, sizeof(digest), digest_hex);
    int written = snprintf(hash_out, hash_out_len, PBKDF2_PREFIX "$%u$%s$%s",
                           PBKDF2_ITERATIONS, salt_hex, digest_hex);
    if (written < 0 || (size_t)written >= hash_out_len) {
        hash_out[0] = '\0';
        return -1;
    }
    return 0;
}

static int verify_legacy_salted_sha256(const char *password, const char *stored_hash) {
    if (strlen(stored_hash) != 97U || stored_hash[32] != ':') return 0;

    unsigned char salt[PASSWORD_SALT_LEN];
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    if (hex_to_bytes(stored_hash, 32U, salt, sizeof(salt)) != 0) return 0;

    EVP_MD_CTX *context = EVP_MD_CTX_new();
    if (!context) return 0;
    int success = EVP_DigestInit_ex(context, EVP_sha256(), NULL) == 1 &&
                  EVP_DigestUpdate(context, salt, sizeof(salt)) == 1 &&
                  EVP_DigestUpdate(context, password, strlen(password)) == 1 &&
                  EVP_DigestFinal_ex(context, digest, &digest_len) == 1;
    EVP_MD_CTX_free(context);
    if (!success || digest_len != PASSWORD_DIGEST_LEN) return 0;

    char digest_hex[PASSWORD_DIGEST_LEN * 2U + 1U];
    bytes_to_hex(digest, PASSWORD_DIGEST_LEN, digest_hex);
    return secure_string_equal(digest_hex, stored_hash + 33U);
}

int verify_password(const char *password, const char *stored_hash) {
    if (!password || !stored_hash) return 0;

    unsigned int iterations = 0;
    unsigned char salt[PASSWORD_SALT_LEN];
    unsigned char expected_digest[PASSWORD_DIGEST_LEN];
    if (parse_pbkdf2_hash(stored_hash, &iterations, salt, expected_digest) == 0) {
        unsigned char actual_digest[PASSWORD_DIGEST_LEN];
        if (derive_pbkdf2(password, salt, iterations, actual_digest) != 0) return 0;
        return CRYPTO_memcmp(actual_digest, expected_digest, sizeof(actual_digest)) == 0;
    }

    if (strchr(stored_hash, ':')) return verify_legacy_salted_sha256(password, stored_hash);
    // Temporary compatibility for rows that are migrated at startup.
    return secure_string_equal(password, stored_hash);
}

int password_needs_rehash(const char *stored_hash) {
    if (!stored_hash) return 1;

    unsigned int iterations = 0;
    unsigned char salt[PASSWORD_SALT_LEN];
    unsigned char digest[PASSWORD_DIGEST_LEN];
    if (parse_pbkdf2_hash(stored_hash, &iterations, salt, digest) != 0) return 1;
    return iterations < PBKDF2_ITERATIONS;
}
