#include "utils.h"
#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "check failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            failures++; \
        } \
    } while (0)

int main(void) {
    char slug[9];
    CHECK(generate_random_slug(slug, 8U) == 0);
    CHECK(strlen(slug) == 8U);
    CHECK(is_valid_slug(slug, 31U));

    char normalized[32];
    slugify("Hello, C URL_shortener!", normalized, (int)sizeof(normalized));
    CHECK(strcmp(normalized, "hello-c-url-shortener") == 0);
    CHECK(is_valid_slug("Alpha-9", 31U));
    CHECK(!is_valid_slug("-bad", 31U));
    CHECK(!is_valid_slug("bad-", 31U));
    CHECK(!is_valid_slug("bad/path", 31U));

    CHECK(is_valid_http_url("https://example.test/path?q=value", 2048U));
    CHECK(is_valid_http_url("HTTP://example.test", 2048U));
    CHECK(!is_valid_http_url("ftp://example.test", 2048U));
    CHECK(!is_valid_http_url("https://", 2048U));
    CHECK(!is_valid_http_url("https://user@example.test", 2048U));
    CHECK(!is_valid_http_url("https://example.test/\r\nX-Test: injected", 2048U));
    CHECK(!is_valid_http_url("https://example.test/\"<script>", 2048U));

    char password_hash[128];
    CHECK(hash_password("correct horse battery staple", password_hash, sizeof(password_hash)) == 0);
    CHECK(strncmp(password_hash, "pbkdf2-sha256$600000$", 20U) == 0);
    CHECK(verify_password("correct horse battery staple", password_hash));
    CHECK(!verify_password("wrong password", password_hash));
    CHECK(!password_needs_rehash(password_hash));
    CHECK(verify_password("legacy-password", "legacy-password"));
    CHECK(password_needs_rehash("legacy-password"));
    CHECK(!verify_password("legacy-password", "pbkdf2-sha256$not-valid"));

    if (failures != 0) return 1;
    puts("test_utils: ok");
    return 0;
}
