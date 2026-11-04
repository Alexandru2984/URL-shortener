#include "utils.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define MAX_LOG_FILE_BYTES (10L * 1024L * 1024L)

void slugify(const char *input, char *output, int max_len) {
    if (!input || !output || max_len < 1) return;

    int i = 0, j = 0;
    while (input[i] && j < max_len - 1) {
        char c = input[i];
        if (isalnum((unsigned char)c)) {
            output[j++] = (char)tolower((unsigned char)c);
        } else if (c == ' ' || c == '-' || c == '_') {
            if (j > 0 && output[j - 1] != '-') {
                output[j++] = '-';
            }
        }
        i++;
    }
    if (j > 0 && output[j - 1] == '-') j--;
    output[j] = '\0';
}

int is_valid_slug(const char *slug, size_t max_len) {
    if (!slug || max_len == 0) return 0;

    size_t len = strnlen(slug, max_len + 1U);
    if (len == 0 || len > max_len || slug[0] == '-' || slug[len - 1U] == '-') return 0;

    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)slug[i];
        if (!isalnum(c) && c != '-') return 0;
    }
    return 1;
}

int is_valid_tag_list(const char *tags, size_t max_len) {
    if (!tags) return 1;
    size_t len = strnlen(tags, max_len + 1U);
    if (len == 0) return 1;
    if (len > max_len) return 0;

    size_t token_len = 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)tags[i];
        if (c == ',') {
            if (token_len == 0 || token_len > 24U) return 0;
            token_len = 0;
        } else if (isalnum(c) || c == '-' || c == '_') {
            token_len++;
            if (token_len > 24U) return 0;
        } else {
            return 0;
        }
    }
    return token_len > 0 && token_len <= 24U;
}

int is_valid_idempotency_key(const char *key, size_t max_len) {
    if (!key || max_len == 0) return 0;
    size_t len = strnlen(key, max_len + 1U);
    if (len < 4U || len > max_len) return 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)key[i];
        if (!isalnum(c) && c != '-' && c != '_' && c != '.' && c != ':') return 0;
    }
    return 1;
}

static int append_urlencoded_param(char *buf, size_t buf_size, size_t *pos,
                                   const char *sep, const char *key, const char *val) {
    int n = snprintf(buf + *pos, buf_size - *pos, "%s%s=", sep, key);
    if (n < 0 || *pos + (size_t)n >= buf_size) return -1;
    *pos += (size_t)n;

    static const char hex[] = "0123456789ABCDEF";
    for (size_t i = 0; val[i] != '\0'; i++) {
        unsigned char c = (unsigned char)val[i];
        if (c < 0x20U || c > 0x7eU) return -1;
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            if (*pos + 2U >= buf_size) return -1;
            buf[(*pos)++] = (char)c;
        } else if (c == ' ') {
            if (*pos + 2U >= buf_size) return -1;
            buf[(*pos)++] = '+';
        } else {
            if (*pos + 4U >= buf_size) return -1;
            buf[(*pos)++] = '%';
            buf[(*pos)++] = hex[c >> 4U];
            buf[(*pos)++] = hex[c & 0x0fU];
        }
    }
    buf[*pos] = '\0';
    return 0;
}

int append_utm_params(const char *url, const char *utm_source,
                      const char *utm_medium, const char *utm_campaign,
                      char *url_out, size_t url_out_len) {
    if (!url || !url_out || url_out_len == 0) return -1;

    int has_src = utm_source && utm_source[0] != '\0';
    int has_med = utm_medium && utm_medium[0] != '\0';
    int has_cmp = utm_campaign && utm_campaign[0] != '\0';

    if (!has_src && !has_med && !has_cmp) {
        size_t len = strnlen(url, url_out_len);
        if (len >= url_out_len) return -1;
        memcpy(url_out, url, len + 1U);
        return 0;
    }

    const char *fragment = strchr(url, '#');
    size_t base_len = fragment ? (size_t)(fragment - url) : strlen(url);
    if (base_len >= url_out_len) return -1;

    memcpy(url_out, url, base_len);
    url_out[base_len] = '\0';
    size_t pos = base_len;

    int has_query = memchr(url, '?', base_len) != NULL;
    const char *sep = has_query ? "&" : "?";

    if (has_src) {
        if (append_urlencoded_param(url_out, url_out_len, &pos, sep, "utm_source", utm_source) != 0) return -1;
        sep = "&";
    }
    if (has_med) {
        if (append_urlencoded_param(url_out, url_out_len, &pos, sep, "utm_medium", utm_medium) != 0) return -1;
        sep = "&";
    }
    if (has_cmp) {
        if (append_urlencoded_param(url_out, url_out_len, &pos, sep, "utm_campaign", utm_campaign) != 0) return -1;
    }

    if (fragment) {
        size_t frag_len = strlen(fragment);
        if (pos + frag_len >= url_out_len) return -1;
        memcpy(url_out + pos, fragment, frag_len + 1U);
    }
    return 0;
}

static int extract_url_host_and_port(const char *url, size_t max_len,
                                     char *host_out, size_t host_out_len,
                                     uint16_t *port_out, int *has_explicit_port) {
    if (!url || max_len == 0 || !host_out || host_out_len < 4U) return 0;
    if (port_out) *port_out = 0;
    if (has_explicit_port) *has_explicit_port = 0;

    size_t len = strnlen(url, max_len + 1U);
    if (len == 0 || len > max_len) return 0;

    size_t scheme_len = 0;
    uint16_t default_port = 80U;
    if (strncasecmp(url, "https://", 8U) == 0) {
        scheme_len = 8U;
        default_port = 443U;
    } else if (strncasecmp(url, "http://", 7U) == 0) {
        scheme_len = 7U;
        default_port = 80U;
    } else {
        return 0;
    }

    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)url[i];
        if (c <= 0x20U || c > 0x7eU || c == '\\' ||
            c == '"' || c == '\'' || c == '<' || c == '>') {
            return 0;
        }
    }

    size_t authority_end = scheme_len;
    while (authority_end < len && url[authority_end] != '/' &&
           url[authority_end] != '?' && url[authority_end] != '#') {
        if (url[authority_end] == '@') return 0;
        authority_end++;
    }
    if (authority_end == scheme_len) return 0;

    size_t host_start = scheme_len;
    size_t host_end = authority_end;
    size_t port_start = authority_end;
    int has_port = 0;

    if (url[host_start] == '[') {
        const char *closing = memchr(url + host_start, ']', authority_end - host_start);
        if (!closing) return 0;
        size_t bracket_pos = (size_t)(closing - url);
        if (bracket_pos <= host_start + 1U) return 0;
        host_start = host_start + 1U;
        host_end = bracket_pos;
        if (bracket_pos + 1U < authority_end) {
            if (url[bracket_pos + 1U] != ':') return 0;
            port_start = bracket_pos + 2U;
            has_port = 1;
        }
    } else {
        const char *colon = memchr(url + host_start, ':', authority_end - host_start);
        if (colon) {
            host_end = (size_t)(colon - url);
            port_start = host_end + 1U;
            has_port = 1;
        }
    }

    size_t host_len = host_end - host_start;
    if (host_len == 0 || host_len >= host_out_len || host_len > 253U) return 0;

    for (size_t i = 0; i < host_len; i++) {
        host_out[i] = (char)tolower((unsigned char)url[host_start + i]);
    }
    host_out[host_len] = '\0';

    if (has_port) {
        if (port_start >= authority_end) return 0;
        size_t port_len = authority_end - port_start;
        if (port_len > 5U) return 0;
        unsigned long port_val = 0;
        for (size_t i = port_start; i < authority_end; i++) {
            if (!isdigit((unsigned char)url[i])) return 0;
            port_val = port_val * 10UL + (unsigned long)(url[i] - '0');
        }
        if (port_val < 1UL || port_val > 65535UL) return 0;
        if (port_out) *port_out = (uint16_t)port_val;
        if (has_explicit_port) *has_explicit_port = 1;
    } else {
        if (port_out) *port_out = default_port;
    }

    // Validate host syntax (IPv6, IPv4, or RFC 1123 domain name)
    if (strchr(host_out, ':')) {
        struct in6_addr ipv6;
        if (inet_pton(AF_INET6, host_out, &ipv6) != 1) return 0;
        return 1;
    }

    if (host_out[0] == '.' || host_out[host_len - 1U] == '.' ||
        host_out[0] == '-' || host_out[host_len - 1U] == '-') {
        return 0;
    }

    size_t label_len = 0;
    for (size_t i = 0; i < host_len; i++) {
        unsigned char c = (unsigned char)host_out[i];
        if (c == '.') {
            if (label_len == 0 || label_len > 63U || host_out[i - 1U] == '-') return 0;
            label_len = 0;
        } else if (isalnum(c) || c == '-') {
            if (label_len == 0 && c == '-') return 0;
            label_len++;
        } else {
            return 0;
        }
    }
    if (label_len == 0 || label_len > 63U) return 0;
    return 1;
}

int is_valid_http_url(const char *url, size_t max_len) {
    char host[256];
    uint16_t port = 0;
    return extract_url_host_and_port(url, max_len, host, sizeof(host), &port, NULL);
}

static int is_private_or_reserved_ipv4(uint32_t ip_host_order) {
    if ((ip_host_order & 0xFF000000U) == 0x00000000U) return 1; // 0.0.0.0/8
    if ((ip_host_order & 0xFF000000U) == 0x0A000000U) return 1; // 10.0.0.0/8
    if ((ip_host_order & 0xFFC00000U) == 0x64400000U) return 1; // 100.64.0.0/10 (CGNAT)
    if ((ip_host_order & 0xFF000000U) == 0x7F000000U) return 1; // 127.0.0.0/8 (Loopback)
    if ((ip_host_order & 0xFFFF0000U) == 0xA9FE0000U) return 1; // 169.254.0.0/16 (Link-local / Cloud Metadata)
    if ((ip_host_order & 0xFFF00000U) == 0xAC100000U) return 1; // 172.16.0.0/12
    if ((ip_host_order & 0xFFFFFF00U) == 0xC0000000U) return 1; // 192.0.0.0/24
    if ((ip_host_order & 0xFFFFFF00U) == 0xC0000200U) return 1; // 192.0.2.0/24 (TEST-NET-1)
    if ((ip_host_order & 0xFFFF0000U) == 0xC0A80000U) return 1; // 192.168.0.0/16
    if ((ip_host_order & 0xFFFE0000U) == 0xC6120000U) return 1; // 198.18.0.0/15
    if ((ip_host_order & 0xFFFFFF00U) == 0xC6336400U) return 1; // 198.51.100.0/24 (TEST-NET-2)
    if ((ip_host_order & 0xFFFFFF00U) == 0xCB007100U) return 1; // 203.0.113.0/24 (TEST-NET-3)
    if ((ip_host_order & 0xF0000000U) == 0xE0000000U) return 1; // 224.0.0.0/4 (Multicast)
    if ((ip_host_order & 0xF0000000U) == 0xF0000000U) return 1; // 240.0.0.0/4 (Reserved/Broadcast)
    return 0;
}

static int is_private_or_reserved_ipv6(const struct in6_addr *addr) {
    const unsigned char *b = addr->s6_addr;
    // Unspecified :: or Loopback ::1
    int all_zero_prefix = 1;
    for (int i = 0; i < 15; i++) {
        if (b[i] != 0) {
            all_zero_prefix = 0;
            break;
        }
    }
    if (all_zero_prefix && (b[15] == 0 || b[15] == 1)) return 1;

    // IPv4-mapped ::ffff:0:0/96 or IPv4-compatible
    int is_v4_mapped = 1;
    for (int i = 0; i < 10; i++) {
        if (b[i] != 0) {
            is_v4_mapped = 0;
            break;
        }
    }
    if (is_v4_mapped && b[10] == 0xff && b[11] == 0xff) {
        uint32_t v4 = ((uint32_t)b[12] << 24) | ((uint32_t)b[13] << 16) |
                      ((uint32_t)b[14] << 8) | (uint32_t)b[15];
        return is_private_or_reserved_ipv4(v4);
    }

    // Unique local fc00::/7
    if ((b[0] & 0xfeU) == 0xfcU) return 1;
    // Link-local fe80::/10
    if (b[0] == 0xfeU && (b[1] & 0xc0U) == 0x80U) return 1;
    // Multicast ff00::/8
    if (b[0] == 0xffU) return 1;
    // Documentation 2001:db8::/32
    if (b[0] == 0x20U && b[1] == 0x01U && b[2] == 0x0dU && b[3] == 0xb8U) return 1;

    return 0;
}

static int has_suffix(const char *str, const char *suffix) {
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (str_len < suffix_len) return 0;
    return strcmp(str + (str_len - suffix_len), suffix) == 0;
}

int is_safe_redirect_url(const char *url, const char *base_url, size_t max_len) {
    char host[256];
    uint16_t port = 0;
    if (!extract_url_host_and_port(url, max_len, host, sizeof(host), &port, NULL)) {
        return 0;
    }

    // Block self-redirect loops to the shortener's own host
    if (base_url && base_url[0]) {
        char base_host[256];
        uint16_t base_port = 0;
        if (extract_url_host_and_port(base_url, max_len, base_host, sizeof(base_host), &base_port, NULL)) {
            if (strcmp(host, base_host) == 0) return 0;
        }
    }

    // Check IPv6 literal
    if (strchr(host, ':')) {
        struct in6_addr ipv6;
        if (inet_pton(AF_INET6, host, &ipv6) != 1) return 0;
        if (is_private_or_reserved_ipv6(&ipv6)) return 0;
        return 1;
    }

    // Check IPv4 literal
    struct in_addr ipv4;
    if (inet_pton(AF_INET, host, &ipv4) == 1) {
        uint32_t ip_host = ntohl(ipv4.s_addr);
        if (is_private_or_reserved_ipv4(ip_host)) return 0;
        return 1;
    }

    // Block localhost and internal pseudo-TLDs
    if (strcmp(host, "localhost") == 0 ||
        has_suffix(host, ".localhost") ||
        has_suffix(host, ".local") ||
        has_suffix(host, ".internal") ||
        has_suffix(host, ".home.arpa") ||
        has_suffix(host, ".invalid")) {
        return 0;
    }

    // Require at least one dot and a valid alphabetic TLD (>= 2 chars)
    // This also defeats decimal/hex/octal IPv4 SSRF bypasses like http://2130706433 or http://127.1
    const char *last_dot = strrchr(host, '.');
    if (!last_dot || last_dot == host) return 0;
    const char *tld = last_dot + 1U;
    size_t tld_len = strlen(tld);
    if (tld_len < 2U) return 0;
    for (size_t i = 0; i < tld_len; i++) {
        if (!isalpha((unsigned char)tld[i])) return 0;
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
            OPENSSL_cleanse(random_bytes, sizeof(random_bytes));
            slug_out[0] = '\0';
            return -1;
        }
        for (size_t i = 0; i < sizeof(random_bytes) && written < len; i++) {
            if ((unsigned int)random_bytes[i] >= unbiased_limit) continue;
            slug_out[written++] = charset[random_bytes[i] % charset_len];
        }
    }
    OPENSSL_cleanse(random_bytes, sizeof(random_bytes));
    slug_out[len] = '\0';
    return 0;
}

static void bytes_to_hex(const unsigned char *bytes, size_t len, char *hex_out) {
    static const char digits[] = "0123456789abcdef";
    for (size_t i = 0; i < len; i++) {
        hex_out[i * 2U] = digits[bytes[i] >> 4U];
        hex_out[i * 2U + 1U] = digits[bytes[i] & 0x0fU];
    }
    hex_out[len * 2U] = '\0';
}

int generate_api_token(char *token_out, size_t token_out_len,
                       char *prefix_out, size_t prefix_out_len) {
    // Format: csh_<36 random base62 chars> (40 chars total)
    if (!token_out || token_out_len < 42U || !prefix_out || prefix_out_len < 10U) return -1;
    memcpy(token_out, "csh_", 4U);
    if (generate_random_slug(token_out + 4U, 36U) != 0) {
        token_out[0] = '\0';
        prefix_out[0] = '\0';
        return -1;
    }
    memcpy(prefix_out, token_out, 8U);
    prefix_out[8] = '\0';
    return 0;
}

int hash_api_token(const char *token, char *hash_out, size_t hash_out_len) {
    if (!token || !token[0] || !hash_out || hash_out_len < 65U) return -1;

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) return -1;

    int ok = EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) == 1 &&
             EVP_DigestUpdate(ctx, token, strlen(token)) == 1 &&
             EVP_DigestFinal_ex(ctx, digest, &digest_len) == 1 &&
             digest_len == 32U;
    EVP_MD_CTX_free(ctx);
    if (!ok) {
        OPENSSL_cleanse(digest, sizeof(digest));
        return -1;
    }

    bytes_to_hex(digest, 32U, hash_out);
    OPENSSL_cleanse(digest, sizeof(digest));
    return 0;
}

void classify_user_agent(const char *user_agent, char *device_out, size_t device_out_len) {
    if (!device_out || device_out_len < 8U) return;
    if (!user_agent || !user_agent[0]) {
        snprintf(device_out, device_out_len, "%s", "unknown");
        return;
    }

    char lower[256];
    size_t len = strnlen(user_agent, sizeof(lower) - 1U);
    for (size_t i = 0; i < len; i++) {
        lower[i] = (char)tolower((unsigned char)user_agent[i]);
    }
    lower[len] = '\0';

    if (strstr(lower, "bot") || strstr(lower, "spider") || strstr(lower, "crawl") ||
        strstr(lower, "slurp") || strstr(lower, "facebookexternalhit") ||
        strstr(lower, "whatsapp") || strstr(lower, "telegrambot") ||
        strstr(lower, "discordbot") || strstr(lower, "twitterbot")) {
        snprintf(device_out, device_out_len, "%s", "bot");
    } else if (strstr(lower, "curl/") || strstr(lower, "wget/") ||
               strstr(lower, "httpie/") || strstr(lower, "python-requests") ||
               strstr(lower, "go-http-client") || strstr(lower, "postman")) {
        snprintf(device_out, device_out_len, "%s", "cli");
    } else if (strstr(lower, "mobi") || strstr(lower, "android") ||
               strstr(lower, "iphone") || strstr(lower, "ipad") ||
               strstr(lower, "ipod")) {
        snprintf(device_out, device_out_len, "%s", "mobile");
    } else if (strstr(lower, "mozilla/") || strstr(lower, "chrome/") ||
               strstr(lower, "safari/") || strstr(lower, "firefox/") ||
               strstr(lower, "edg/") || strstr(lower, "opera")) {
        snprintf(device_out, device_out_len, "%s", "desktop");
    } else {
        snprintf(device_out, device_out_len, "%s", "unknown");
    }
}

void classify_referrer(const char *referer, const char *base_url,
                       char *domain_out, size_t domain_out_len) {
    if (!domain_out || domain_out_len < 8U) return;
    if (!referer || !referer[0]) {
        snprintf(domain_out, domain_out_len, "%s", "direct");
        return;
    }

    char host[256];
    uint16_t port = 0;
    if (!extract_url_host_and_port(referer, 2048U, host, sizeof(host), &port, NULL)) {
        snprintf(domain_out, domain_out_len, "%s", "direct");
        return;
    }

    if (base_url && base_url[0]) {
        char base_host[256];
        uint16_t base_port = 0;
        if (extract_url_host_and_port(base_url, 512U, base_host, sizeof(base_host), &base_port, NULL) &&
            strcmp(host, base_host) == 0) {
            snprintf(domain_out, domain_out_len, "%s", "self");
            return;
        }
    }

    const char *clean_host = host;
    if (strncmp(clean_host, "www.", 4U) == 0 && clean_host[4] != '\0') {
        clean_host += 4U;
    }
    snprintf(domain_out, domain_out_len, "%.*s", (int)(domain_out_len - 1U), clean_host);
}

void sanitize_csv_field(const char *input, char *output, size_t output_len) {
    if (!output || output_len == 0) return;
    if (!input || !input[0]) {
        output[0] = '\0';
        return;
    }

    size_t out_idx = 0;
    if (out_idx + 1U < output_len) output[out_idx++] = '"';

    // Mitigate spreadsheet formula injection (=, +, -, @, \t, \r)
    unsigned char first = (unsigned char)input[0];
    if ((first == '=' || first == '+' || first == '-' || first == '@' ||
         first == '\t' || first == '\r') && out_idx + 1U < output_len) {
        output[out_idx++] = '\'';
    }

    for (size_t i = 0; input[i] != '\0' && out_idx + 2U < output_len; i++) {
        unsigned char c = (unsigned char)input[i];
        if (c == '"') {
            if (out_idx + 3U >= output_len) break;
            output[out_idx++] = '"';
            output[out_idx++] = '"';
        } else if (c >= 0x20U && c <= 0x7eU) {
            output[out_idx++] = (char)c;
        } else {
            output[out_idx++] = ' ';
        }
    }

    if (out_idx + 1U < output_len) output[out_idx++] = '"';
    output[out_idx] = '\0';
}

void html_escape(const char *input, char *output, size_t output_len) {
    if (!output || output_len == 0) return;
    if (!input) {
        output[0] = '\0';
        return;
    }

    size_t out_idx = 0;
    for (size_t i = 0; input[i] != '\0' && out_idx + 1U < output_len; i++) {
        const char *entity = NULL;
        switch (input[i]) {
            case '&': entity = "&amp;"; break;
            case '<': entity = "&lt;"; break;
            case '>': entity = "&gt;"; break;
            case '"': entity = "&quot;"; break;
            case '\'': entity = "&#39;"; break;
            default: break;
        }
        if (entity) {
            size_t entity_len = strlen(entity);
            if (out_idx + entity_len >= output_len) break;
            memcpy(output + out_idx, entity, entity_len);
            out_idx += entity_len;
        } else {
            unsigned char c = (unsigned char)input[i];
            if (c >= 0x20U && c <= 0x7eU) {
                output[out_idx++] = (char)c;
            }
        }
    }
    output[out_idx] = '\0';
}

void sanitize_country_code(const char *raw_code, char *country_out, size_t country_out_len) {
    if (!country_out || country_out_len < 3U) return;
    if (!raw_code || strlen(raw_code) != 2U) {
        snprintf(country_out, country_out_len, "%s", "XX");
        return;
    }
    unsigned char c0 = (unsigned char)raw_code[0];
    unsigned char c1 = (unsigned char)raw_code[1];
    int valid_iso = isalpha(c0) && isalpha(c1);
    int valid_tor = (toupper(c0) == 'T' && c1 == '1');
    if (!valid_iso && !valid_tor) {
        snprintf(country_out, country_out_len, "%s", "XX");
        return;
    }
    country_out[0] = (char)toupper(c0);
    country_out[1] = (char)toupper(c1);
    country_out[2] = '\0';
}

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

static void rotate_log_if_needed(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0 && st.st_size >= MAX_LOG_FILE_BYTES) {
        char rotated[256];
        int len = snprintf(rotated, sizeof(rotated), "%s.1", filename);
        if (len > 0 && (size_t)len < sizeof(rotated)) {
            (void)rename(filename, rotated);
        }
    }
}

static void sanitize_log_line(char *buffer) {
    if (!buffer) return;
    for (size_t i = 0; buffer[i] != '\0'; i++) {
        unsigned char c = (unsigned char)buffer[i];
        if (c < 0x20U || c == 0x7fU) {
            buffer[i] = ' ';
        }
    }
}

static void log_to_file(const char *filename, const char *level, const char *format, va_list args) {
    char message[1024];
    int formatted = vsnprintf(message, sizeof(message), format, args);
    if (formatted < 0) return;
    sanitize_log_line(message);

    pthread_mutex_lock(&log_mutex);

    rotate_log_if_needed(filename);
    int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC | O_NOFOLLOW, 0640);
    if (fd < 0) {
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    FILE *f = fdopen(fd, "a");
    if (!f) {
        close(fd);
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    time_t now;
    time(&now);
    struct tm tm_buf;
    const struct tm *tm_info = localtime_r(&now, &tm_buf);
    char time_buf[26];
    if (tm_info) {
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
    } else {
        snprintf(time_buf, sizeof(time_buf), "%s", "unknown-time");
    }

    fprintf(f, "[%s] [%s] %s\n", time_buf, level, message);
    fclose(f);

    pthread_mutex_unlock(&log_mutex);
}

void log_message(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_to_file("logs/access.log", "INFO", format, args);
    va_end(args);

    char message[1024];
    va_start(args, format);
    if (vsnprintf(message, sizeof(message), format, args) >= 0) {
        sanitize_log_line(message);
        printf("[INFO] %s\n", message);
    }
    va_end(args);
}

void log_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_to_file("logs/error.log", "ERROR", format, args);
    va_end(args);

    char message[1024];
    va_start(args, format);
    if (vsnprintf(message, sizeof(message), format, args) >= 0) {
        sanitize_log_line(message);
        fprintf(stderr, "[ERROR] %s\n", message);
    }
    va_end(args);
}

// --- Password hashing (PBKDF2-HMAC-SHA-256) ---

#define PASSWORD_SALT_LEN 16U
#define PASSWORD_DIGEST_LEN 32U
#define PBKDF2_ITERATIONS 600000U
#define PASSWORD_HASH_MAX_LEN 128U
#define PBKDF2_PREFIX "pbkdf2-sha256"

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
        OPENSSL_cleanse(salt, sizeof(salt));
        OPENSSL_cleanse(digest, sizeof(digest));
        hash_out[0] = '\0';
        return -1;
    }

    char salt_hex[PASSWORD_SALT_LEN * 2U + 1U];
    char digest_hex[PASSWORD_DIGEST_LEN * 2U + 1U];
    bytes_to_hex(salt, sizeof(salt), salt_hex);
    bytes_to_hex(digest, sizeof(digest), digest_hex);
    OPENSSL_cleanse(salt, sizeof(salt));
    OPENSSL_cleanse(digest, sizeof(digest));

    int written = snprintf(hash_out, hash_out_len, PBKDF2_PREFIX "$%u$%s$%s",
                           PBKDF2_ITERATIONS, salt_hex, digest_hex);
    OPENSSL_cleanse(salt_hex, sizeof(salt_hex));
    OPENSSL_cleanse(digest_hex, sizeof(digest_hex));
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
    OPENSSL_cleanse(salt, sizeof(salt));
    if (!success || digest_len != PASSWORD_DIGEST_LEN) {
        OPENSSL_cleanse(digest, sizeof(digest));
        return 0;
    }

    char digest_hex[PASSWORD_DIGEST_LEN * 2U + 1U];
    bytes_to_hex(digest, PASSWORD_DIGEST_LEN, digest_hex);
    OPENSSL_cleanse(digest, sizeof(digest));
    int eq = secure_string_equal(digest_hex, stored_hash + 33U);
    OPENSSL_cleanse(digest_hex, sizeof(digest_hex));
    return eq;
}

int verify_password(const char *password, const char *stored_hash) {
    if (!password || !stored_hash) return 0;

    unsigned int iterations = 0;
    unsigned char salt[PASSWORD_SALT_LEN];
    unsigned char expected_digest[PASSWORD_DIGEST_LEN];
    if (parse_pbkdf2_hash(stored_hash, &iterations, salt, expected_digest) == 0) {
        unsigned char actual_digest[PASSWORD_DIGEST_LEN];
        int derived = derive_pbkdf2(password, salt, iterations, actual_digest);
        int match = (derived == 0 &&
                     CRYPTO_memcmp(actual_digest, expected_digest, sizeof(actual_digest)) == 0);
        OPENSSL_cleanse(salt, sizeof(salt));
        OPENSSL_cleanse(expected_digest, sizeof(expected_digest));
        OPENSSL_cleanse(actual_digest, sizeof(actual_digest));
        return match;
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
    int parsed = parse_pbkdf2_hash(stored_hash, &iterations, salt, digest);
    OPENSSL_cleanse(salt, sizeof(salt));
    OPENSSL_cleanse(digest, sizeof(digest));
    if (parsed != 0) return 1;
    return iterations < PBKDF2_ITERATIONS;
}
