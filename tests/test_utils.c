#include "utils.h"
#include "qr.h"
#include <stdio.h>
#include <stdlib.h>
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
    CHECK(!is_valid_http_url("https://:8080/path", 2048U));
    CHECK(!is_valid_http_url("https://example.test:99999/path", 2048U));
    CHECK(!is_valid_http_url("https://user@example.test", 2048U));
    CHECK(!is_valid_http_url("https://example.test/\r\nX-Test: injected", 2048U));
    CHECK(!is_valid_http_url("https://example.test/\"<script>", 2048U));

    // SSRF, Private IP, Metadata, and Redirect Loop protection
    CHECK(is_safe_redirect_url("https://example.test/docs", "https://c.micutu.com", 2048U));
    CHECK(is_safe_redirect_url("https://1.1.1.1/dns", "https://c.micutu.com", 2048U));
    CHECK(!is_safe_redirect_url("https://c.micutu.com/loop", "https://c.micutu.com", 2048U));
    CHECK(!is_safe_redirect_url("http://127.0.0.1:8086/admin", "https://c.micutu.com", 2048U));
    CHECK(!is_safe_redirect_url("http://localhost/admin", "https://c.micutu.com", 2048U));
    CHECK(!is_safe_redirect_url("http://sub.localhost/admin", "https://c.micutu.com", 2048U));
    CHECK(!is_safe_redirect_url("http://169.254.169.254/latest/meta-data/", "https://c.micutu.com", 2048U));
    CHECK(!is_safe_redirect_url("http://10.0.0.1/secret", "https://c.micutu.com", 2048U));
    CHECK(!is_safe_redirect_url("http://192.168.1.1/router", "https://c.micutu.com", 2048U));
    CHECK(!is_safe_redirect_url("http://172.16.5.4/internal", "https://c.micutu.com", 2048U));
    CHECK(!is_safe_redirect_url("http://[::1]:8080/", "https://c.micutu.com", 2048U));
    CHECK(!is_safe_redirect_url("http://[::ffff:127.0.0.1]/", "https://c.micutu.com", 2048U));
    CHECK(!is_safe_redirect_url("http://2130706433/", "https://c.micutu.com", 2048U));
    CHECK(!is_safe_redirect_url("http://intranet/login", "https://c.micutu.com", 2048U));

    // Tag validation
    CHECK(is_valid_tag_list("work,promo,2026-q4", 128U));
    CHECK(!is_valid_tag_list("work,,promo", 128U));
    CHECK(!is_valid_tag_list("bad<tag>", 128U));

    // Idempotency key & UTM parameter appending
    CHECK(is_valid_idempotency_key("release_2026-v1.0:prod", 64U));
    CHECK(!is_valid_idempotency_key("bad key with spaces", 64U));
    CHECK(!is_valid_idempotency_key("", 64U));

    char utm_url[512];
    CHECK(append_utm_params("https://example.test/landing", "newsletter", "email", "q4_launch",
                            utm_url, sizeof(utm_url)) == 0);
    CHECK(strcmp(utm_url, "https://example.test/landing?utm_source=newsletter&utm_medium=email&utm_campaign=q4_launch") == 0);

    CHECK(append_utm_params("https://example.test/landing?ref=1#top", "cli", NULL, "v2",
                            utm_url, sizeof(utm_url)) == 0);
    CHECK(strcmp(utm_url, "https://example.test/landing?ref=1&utm_source=cli&utm_campaign=v2#top") == 0);

    // Scoped API token generation & SHA-256 hashing
    char token[64];
    char prefix[16];
    char token_hash[65];
    CHECK(generate_api_token(token, sizeof(token), prefix, sizeof(prefix)) == 0);
    CHECK(strncmp(token, "csh_", 4U) == 0);
    CHECK(strlen(token) == 40U);
    CHECK(strlen(prefix) == 8U);
    CHECK(hash_api_token(token, token_hash, sizeof(token_hash)) == 0);
    CHECK(strlen(token_hash) == 64U);

    // Privacy-preserving classifiers
    char device[16];
    classify_user_agent("Mozilla/5.0 (iPhone; CPU iPhone OS 17_0 like Mac OS X)", device, sizeof(device));
    CHECK(strcmp(device, "mobile") == 0);
    classify_user_agent("curl/8.5.0", device, sizeof(device));
    CHECK(strcmp(device, "cli") == 0);
    classify_user_agent("Mozilla/5.0 (X11; Linux x86_64; rv:128.0) Gecko/20100101 Firefox/128.0", device, sizeof(device));
    CHECK(strcmp(device, "desktop") == 0);

    char domain[64];
    classify_referrer("https://www.github.com/user/repo?token=secret", "https://c.micutu.com", domain, sizeof(domain));
    CHECK(strcmp(domain, "github.com") == 0);
    classify_referrer("https://c.micutu.com/admin", "https://c.micutu.com", domain, sizeof(domain));
    CHECK(strcmp(domain, "self") == 0);
    classify_referrer(NULL, "https://c.micutu.com", domain, sizeof(domain));
    CHECK(strcmp(domain, "direct") == 0);

    // CSV formula injection & HTML escaping
    char csv_out[128];
    sanitize_csv_field("=cmd|' /C calc'!A0", csv_out, sizeof(csv_out));
    CHECK(strcmp(csv_out, "\"'=cmd|' /C calc'!A0\"") == 0);

    char html_out[128];
    html_escape("<script>alert(\"xss\")</script>", html_out, sizeof(html_out));
    CHECK(strcmp(html_out, "&lt;script&gt;alert(&quot;xss&quot;)&lt;/script&gt;") == 0);

    // Pure-C SVG QR Code generator (Versions 1..6)
    char *svg_v2 = qr_generate_svg("https://c.micutu.com/abc12345", 4);
    CHECK(svg_v2 != NULL);
    if (svg_v2) {
        CHECK(strstr(svg_v2, "<svg xmlns=\"http://www.w3.org/2000/svg\"") != NULL);
        CHECK(strstr(svg_v2, "<path fill=\"#0f172a\" d=\"M") != NULL);
        free(svg_v2);
    }

    char *svg_v6 = qr_generate_svg(
        "https://c.micutu.com/very-long-custom-slug-testing-qr-version-6-interleaved-reed-solomon-blocks-1234567890", 2);
    CHECK(svg_v6 != NULL);
    free(svg_v6);

    char *svg_themed = qr_generate_svg_themed("https://c.micutu.com/cyber", 4, "#38bdf8", "#0b1120");
    CHECK(svg_themed != NULL);
    if (svg_themed) {
        CHECK(strstr(svg_themed, "fill=\"#0b1120\"") != NULL);
        CHECK(strstr(svg_themed, "fill=\"#38bdf8\"") != NULL);
        free(svg_themed);
    }
    char *svg_bad_color = qr_generate_svg_themed("https://c.micutu.com/safe", 4, "\"><script>", "bad");
    CHECK(svg_bad_color != NULL);
    if (svg_bad_color) {
        CHECK(strstr(svg_bad_color, "<script>") == NULL);
        CHECK(strstr(svg_bad_color, "fill=\"#0f172a\"") != NULL);
        free(svg_bad_color);
    }

    CHECK(qr_generate_svg("", 4) == NULL);

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
