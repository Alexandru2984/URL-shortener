#include "handlers.h"
#include "db.h"
#include "qr.h"
#include "utils.h"
#include "assets.h"
#include "index_html.h"
#include "password_html.h"
#include "stats_html.h"
#include "inspect_html.h"
#include "error_html.h"
#include "admin_html.h"
#include <arpa/inet.h>
#include <cjson/cJSON.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <openssl/crypto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

static time_t g_start_time = 0;

static long get_uptime_seconds(void) {
    time_t now = time(NULL);
    if (g_start_time == 0) g_start_time = now;
    return now >= g_start_time ? (long)(now - g_start_time) : 0L;
}

#define MAX_URL_LEN 2048U
#define MAX_URL_BUFFER (MAX_URL_LEN + 1U)
#define MAX_SLUG_LEN 32U
#define MAX_SLUG_LENGTH (MAX_SLUG_LEN - 1U)
#define MAX_PASSWORD_LEN 128U
#define MAX_TITLE_LEN 160U
#define MAX_TAGS_LEN 128U
#define MAX_REQUEST_BODY 8192U
#define MAX_TTL_HOURS 8760
#define MAX_CLICKS_LIMIT 1000000
#define MAX_FULL_URL_LEN 512U

#define CREATE_RATE_LIMIT 20
#define UNLOCK_RATE_LIMIT 5
#define STATS_RATE_LIMIT 60
#define ADMIN_RATE_LIMIT 30

static int scoped_rate_limit(const char *scope, const char *ip, int max_per_min) {
    if (!ip || !ip[0]) return -1;
    char key[128];
    snprintf(key, sizeof(key), "%s:%s", scope ? scope : "default", ip);
    return check_rate_limit(key, max_per_min);
}

// Set by main() from the API_KEY and BASE_URL environment variables.
const char *g_api_key = NULL;
const char *g_base_url = "https://c.micutu.com";

struct connection_info_struct {
    char *data;
    size_t data_len;
    int body_too_large;
};

static void add_security_headers(struct MHD_Response *response) {
    MHD_add_response_header(response, "X-Content-Type-Options", "nosniff");
    MHD_add_response_header(response, "X-Frame-Options", "DENY");
    MHD_add_response_header(response, "X-Permitted-Cross-Domain-Policies", "none");
    MHD_add_response_header(response, "Referrer-Policy", "no-referrer");
    MHD_add_response_header(response, "Permissions-Policy", "geolocation=(), microphone=(), camera=()");
    MHD_add_response_header(response, "Cross-Origin-Opener-Policy", "same-origin");
    MHD_add_response_header(response, "Cross-Origin-Resource-Policy", "same-origin");
    MHD_add_response_header(response, "Cache-Control", "no-store, max-age=0");
    MHD_add_response_header(response, "Pragma", "no-cache");
    MHD_add_response_header(response, "Content-Security-Policy",
                            "default-src 'self'; base-uri 'none'; form-action 'self'; "
                            "frame-ancestors 'none'; object-src 'none'; connect-src 'self'; "
                            "img-src 'self' data:; script-src 'self'; style-src 'self'");
}

static int send_json_response(struct MHD_Connection *connection, unsigned int status_code,
                              const char *json_str) {
    if (!json_str) return MHD_NO;

    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(json_str), (void *)json_str, MHD_RESPMEM_MUST_COPY);
    if (!response) return MHD_NO;

    MHD_add_response_header(response, "Content-Type", "application/json; charset=utf-8");
    add_security_headers(response);
    int ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    return ret;
}

static int send_error(struct MHD_Connection *connection, unsigned int status_code,
                      const char *message) {
    cJSON *json = cJSON_CreateObject();
    if (!json || !cJSON_AddStringToObject(json, "error", message)) {
        cJSON_Delete(json);
        return send_json_response(connection, status_code, "{\"error\":\"Request failed\"}");
    }

    char *json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    if (!json_str) return send_json_response(connection, status_code, "{\"error\":\"Request failed\"}");

    int ret = send_json_response(connection, status_code, json_str);
    free(json_str);
    return ret;
}

static int send_html(struct MHD_Connection *connection, unsigned int status_code,
                     const char *html) {
    if (!html) return MHD_NO;

    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(html), (void *)html, MHD_RESPMEM_PERSISTENT);
    if (!response) return MHD_NO;

    MHD_add_response_header(response, "Content-Type", "text/html; charset=utf-8");
    add_security_headers(response);
    int ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    return ret;
}

static int send_html_dynamic(struct MHD_Connection *connection, unsigned int status_code,
                             char *html) {
    if (!html) return MHD_NO;

    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(html), html, MHD_RESPMEM_MUST_FREE);
    if (!response) {
        free(html);
        return MHD_NO;
    }

    MHD_add_response_header(response, "Content-Type", "text/html; charset=utf-8");
    add_security_headers(response);
    int ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    return ret;
}

static int send_dynamic_buffer(struct MHD_Connection *connection, unsigned int status_code,
                               char *buffer, const char *content_type,
                               const char *content_disposition) {
    if (!buffer) return MHD_NO;

    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(buffer), buffer, MHD_RESPMEM_MUST_FREE);
    if (!response) {
        free(buffer);
        return MHD_NO;
    }

    MHD_add_response_header(response, "Content-Type", content_type);
    if (content_disposition) {
        MHD_add_response_header(response, "Content-Disposition", content_disposition);
    }
    add_security_headers(response);
    int ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    return ret;
}

static int send_text_response(struct MHD_Connection *connection, unsigned int status_code,
                              const char *text, const char *content_type) {
    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(text), (void *)text, MHD_RESPMEM_PERSISTENT);
    if (!response) return MHD_NO;

    MHD_add_response_header(response, "Content-Type", content_type);
    add_security_headers(response);
    int ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    return ret;
}

static int send_empty_response(struct MHD_Connection *connection, unsigned int status_code) {
    static const char empty[] = "";
    struct MHD_Response *response = MHD_create_response_from_buffer(
        0, (void *)empty, MHD_RESPMEM_PERSISTENT);
    if (!response) return MHD_NO;

    add_security_headers(response);
    int ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    return ret;
}

static int send_error_page(struct MHD_Connection *connection, unsigned int code,
                           const char *title, const char *desc) {
    char page_title[128];
    int title_len = snprintf(page_title, sizeof(page_title), "%u - %s", code, title);
    if (title_len < 0 || (size_t)title_len >= sizeof(page_title)) {
        return send_error(connection, code, "Request failed");
    }

    int html_len = snprintf(NULL, 0, ERROR_HTML_TEMPLATE, page_title, (int)code, title, desc);
    if (html_len < 0) return send_error(connection, code, "Request failed");

    char *html = malloc((size_t)html_len + 1U);
    if (!html) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error");
    snprintf(html, (size_t)html_len + 1U, ERROR_HTML_TEMPLATE, page_title, (int)code, title, desc);
    return send_html_dynamic(connection, code, html);
}

static int api_key_is_configured(void) {
    return g_api_key && g_api_key[0] != '\0';
}

static int check_admin_api_key(struct MHD_Connection *connection) {
    if (!api_key_is_configured()) return 0;

    const char *provided = MHD_lookup_connection_value(
        connection, MHD_HEADER_KIND, "X-API-Key");
    if (!provided) return 0;

    size_t expected_len = strlen(g_api_key);
    size_t provided_len = strlen(provided);
    if (expected_len != provided_len) return 0;
    return CRYPTO_memcmp(g_api_key, provided, expected_len) == 0;
}

static int check_create_permission(struct MHD_Connection *connection,
                                   char *actor_out, size_t actor_len) {
    if (check_admin_api_key(connection)) {
        if (actor_out && actor_len > 0) snprintf(actor_out, actor_len, "%s", "admin");
        return 1;
    }

    const char *provided = MHD_lookup_connection_value(
        connection, MHD_HEADER_KIND, "X-API-Key");
    if (!provided) {
        provided = MHD_lookup_connection_value(
            connection, MHD_HEADER_KIND, "X-Creator-Token");
    }
    if (!provided) return 0;
    return verify_creator_token(provided, actor_out, actor_len);
}

static int is_reserved_slug(const char *slug) {
    static const char *const reserved[] = {
        "admin", "api", "assets", "badge", "bio", "docs", "favicon.ico", "favicon.svg",
        "health", "inspect", "manifest.json", "metrics", "openapi.json", "p", "qr",
        "robots.txt", "shorten", "sitemap.xml", "stats", "unlock"
    };

    for (size_t i = 0; i < sizeof(reserved) / sizeof(reserved[0]); i++) {
        if (strcasecmp(slug, reserved[i]) == 0) return 1;
    }
    return 0;
}

static int canonicalize_ip(const char *candidate, char *output, socklen_t output_len) {
    if (!candidate || !output || output_len == 0) return 0;

    struct in_addr ipv4;
    if (inet_pton(AF_INET, candidate, &ipv4) == 1) {
        return inet_ntop(AF_INET, &ipv4, output, output_len) != NULL;
    }

    struct in6_addr ipv6;
    if (inet_pton(AF_INET6, candidate, &ipv6) == 1) {
        return inet_ntop(AF_INET6, &ipv6, output, output_len) != NULL;
    }
    return 0;
}

static void get_client_ip(struct MHD_Connection *connection, char *output, socklen_t output_len) {
    const char *forwarded_ip = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "X-Real-IP");
    if (canonicalize_ip(forwarded_ip, output, output_len)) return;

    const union MHD_ConnectionInfo *info = MHD_get_connection_info(
        connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);
    if (info && info->client_addr) {
        if (info->client_addr->sa_family == AF_INET) {
            const struct sockaddr_in *address = (const struct sockaddr_in *)info->client_addr;
            if (inet_ntop(AF_INET, &address->sin_addr, output, output_len)) return;
        } else if (info->client_addr->sa_family == AF_INET6) {
            const struct sockaddr_in6 *address = (const struct sockaddr_in6 *)info->client_addr;
            if (inet_ntop(AF_INET6, &address->sin6_addr, output, output_len)) return;
        }
    }

    snprintf(output, output_len, "%s", "unknown");
}

static void record_request_visit(struct MHD_Connection *connection,
                                 const char *slug, const char *client_ip) {
    const char *ua = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "User-Agent");
    const char *ref = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Referer");
    const char *cc = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "CF-IPCountry");
    if (!cc || !cc[0]) {
        cc = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "X-Country-Code");
    }

    char device[16];
    char ref_domain[128];
    classify_user_agent(ua, device, sizeof(device));
    classify_referrer(ref, g_base_url, ref_domain, sizeof(ref_domain));

    if (record_visit_geo(slug, client_ip, device, ref_domain, cc) != 0) {
        log_error("Could not record visit for %s", slug);
    }
}

static int is_json_content_type(struct MHD_Connection *connection) {
    const char *content_type = MHD_lookup_connection_value(
        connection, MHD_HEADER_KIND, "Content-Type");
    const char json_type[] = "application/json";
    size_t json_type_len = sizeof(json_type) - 1U;

    if (!content_type || strncasecmp(content_type, json_type, json_type_len) != 0) return 0;
    return content_type[json_type_len] == '\0' || content_type[json_type_len] == ';' ||
           isspace((unsigned char)content_type[json_type_len]);
}

static cJSON *parse_json_body(const struct connection_info_struct *info) {
    if (!info || !info->data || info->data_len == 0) return NULL;

    const char *parse_end = NULL;
    cJSON *json = cJSON_ParseWithLengthOpts(info->data, info->data_len, &parse_end, 0);
    if (!json || !cJSON_IsObject(json)) {
        cJSON_Delete(json);
        return NULL;
    }

    while (parse_end && (size_t)(parse_end - info->data) < info->data_len &&
           isspace((unsigned char)*parse_end)) {
        parse_end++;
    }
    if (!parse_end || (size_t)(parse_end - info->data) != info->data_len) {
        cJSON_Delete(json);
        return NULL;
    }
    return json;
}

static int get_optional_string(cJSON *json, const char *key, size_t max_len,
                               const char **out_val) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(json, key);
    *out_val = NULL;
    if (!item || cJSON_IsNull(item)) return 0;
    if (!cJSON_IsString(item) || !item->valuestring) return -1;

    size_t len = strnlen(item->valuestring, max_len + 1U);
    if (len > max_len) return -1;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)item->valuestring[i];
        if (c < 0x20U || c == 0x7fU) return -1;
    }
    if (len > 0) *out_val = item->valuestring;
    return 0;
}

static int get_optional_int_range(cJSON *json, const char *key,
                                  int min_val, int max_val, int default_val,
                                  int *out_val) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(json, key);
    *out_val = default_val;
    if (!item || cJSON_IsNull(item)) return 0;
    if (!cJSON_IsNumber(item) || !isfinite(item->valuedouble) ||
        floor(item->valuedouble) != item->valuedouble ||
        item->valuedouble < (double)min_val || item->valuedouble > (double)max_val) {
        return -1;
    }
    *out_val = (int)item->valuedouble;
    return 0;
}

static int send_short_link_created_json(struct MHD_Connection *connection,
                                        unsigned int status_code, const char *slug,
                                        int idempotent_replay) {
    char full_url[MAX_FULL_URL_LEN];
    char qr_url[MAX_FULL_URL_LEN];
    char stats_url[MAX_FULL_URL_LEN];
    char inspect_url[MAX_FULL_URL_LEN];
    int full_url_len = snprintf(full_url, sizeof(full_url), "%s/%s", g_base_url, slug);
    snprintf(qr_url, sizeof(qr_url), "%s/qr/%s.svg", g_base_url, slug);
    snprintf(stats_url, sizeof(stats_url), "%s/stats/%s", g_base_url, slug);
    snprintf(inspect_url, sizeof(inspect_url), "%s/inspect/%s", g_base_url, slug);
    if (full_url_len < 0 || (size_t)full_url_len >= sizeof(full_url)) {
        log_error("Configured base URL is too long");
        return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Service configuration error");
    }

    cJSON *response_json = cJSON_CreateObject();
    if (!response_json ||
        !cJSON_AddStringToObject(response_json, "short_url", full_url) ||
        !cJSON_AddStringToObject(response_json, "slug", slug) ||
        !cJSON_AddStringToObject(response_json, "qr_url", qr_url) ||
        !cJSON_AddStringToObject(response_json, "stats_url", stats_url) ||
        !cJSON_AddStringToObject(response_json, "inspect_url", inspect_url) ||
        !cJSON_AddBoolToObject(response_json, "idempotent_replay", idempotent_replay)) {
        cJSON_Delete(response_json);
        return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error");
    }
    char *response_text = cJSON_PrintUnformatted(response_json);
    cJSON_Delete(response_json);
    if (!response_text) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error");

    int ret = send_json_response(connection, status_code, response_text);
    free(response_text);
    return ret;
}

static int handle_shorten(struct MHD_Connection *connection, const char *client_ip,
                          cJSON *json) {
    char actor[64];
    if (!check_create_permission(connection, actor, sizeof(actor))) {
        (void)record_audit_event("auth.failed", "anonymous", "/shorten", client_ip, "Invalid API key or token");
        return send_error(connection, MHD_HTTP_UNAUTHORIZED, "Invalid or missing API key");
    }
    if (scoped_rate_limit("create", client_ip, CREATE_RATE_LIMIT) != 0) {
        return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
    }

    const char *idemp_header = MHD_lookup_connection_value(
        connection, MHD_HEADER_KIND, "Idempotency-Key");
    if (idemp_header && idemp_header[0]) {
        if (!is_valid_idempotency_key(idemp_header, 64U)) {
            return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid Idempotency-Key header");
        }
        char existing_slug[MAX_SLUG_LEN];
        if (find_slug_by_idempotency_key(idemp_header, existing_slug, sizeof(existing_slug)) == 0) {
            return send_short_link_created_json(connection, MHD_HTTP_OK, existing_slug, 1);
        }
    }

    cJSON *target_url = cJSON_GetObjectItemCaseSensitive(json, "url");
    cJSON *custom_slug = cJSON_GetObjectItemCaseSensitive(json, "custom_slug");
    if (!cJSON_IsString(target_url) || !target_url->valuestring ||
        !is_safe_redirect_url(target_url->valuestring, g_base_url, MAX_URL_LEN)) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST,
                          "A valid public HTTP or HTTPS URL is required");
    }

    const char *utm_source = NULL;
    const char *utm_medium = NULL;
    const char *utm_campaign = NULL;
    if (get_optional_string(json, "utm_source", 64U, &utm_source) != 0 ||
        get_optional_string(json, "utm_medium", 64U, &utm_medium) != 0 ||
        get_optional_string(json, "utm_campaign", 64U, &utm_campaign) != 0) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid UTM parameters");
    }

    char final_url[MAX_URL_BUFFER];
    if (append_utm_params(target_url->valuestring, utm_source, utm_medium, utm_campaign,
                          final_url, sizeof(final_url)) != 0 ||
        !is_safe_redirect_url(final_url, g_base_url, MAX_URL_LEN)) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "Resulting URL with UTM parameters is invalid or too long");
    }

    struct link_options opts;
    memset(&opts, 0, sizeof(opts));
    opts.idempotency_key = (idemp_header && idemp_header[0]) ? idemp_header : NULL;

    if (get_optional_int_range(json, "ttl_hours", 0, MAX_TTL_HOURS, 0, &opts.ttl_hours) != 0) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "ttl_hours must be a whole number from 0 to 8760");
    }
    if (get_optional_int_range(json, "max_clicks", 0, MAX_CLICKS_LIMIT, 0, &opts.max_clicks) != 0) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "max_clicks must be between 0 and 1000000");
    }
    if (get_optional_int_range(json, "activates_in_hours", 0, MAX_TTL_HOURS, 0, &opts.activates_in_hours) != 0) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "activates_in_hours must be between 0 and 8760");
    }
    if (get_optional_string(json, "password", MAX_PASSWORD_LEN, &opts.password) != 0) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "Password is too long or invalid");
    }
    if (get_optional_string(json, "title", MAX_TITLE_LEN, &opts.title) != 0) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "Title is too long or invalid");
    }
    if (get_optional_string(json, "tags", MAX_TAGS_LEN, &opts.tags) != 0 ||
        !is_valid_tag_list(opts.tags, MAX_TAGS_LEN)) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "Tags must be comma-separated alphanumeric labels");
    }
    if (get_optional_string(json, "mobile_url", MAX_URL_LEN, &opts.mobile_url) != 0 ||
        (opts.mobile_url && !is_safe_redirect_url(opts.mobile_url, g_base_url, MAX_URL_LEN))) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "mobile_url must be a valid public HTTP or HTTPS URL");
    }
    if (get_optional_string(json, "alt_url", MAX_URL_LEN, &opts.alt_url) != 0 ||
        (opts.alt_url && !is_safe_redirect_url(opts.alt_url, g_base_url, MAX_URL_LEN))) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "alt_url must be a valid public HTTP or HTTPS URL");
    }
    if (get_optional_int_range(json, "alt_weight", 0, 99, 0, &opts.alt_weight) != 0) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "alt_weight must be an integer between 0 and 99");
    }
    if (get_optional_int_range(json, "redirect_code", 301, 308, 302, &opts.redirect_code) != 0 ||
        (opts.redirect_code != 301 && opts.redirect_code != 302 &&
         opts.redirect_code != 307 && opts.redirect_code != 308)) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "redirect_code must be 301, 302, 307, or 308");
    }
    if (get_optional_string(json, "geo_country", 4U, &opts.geo_country) != 0) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "geo_country must be a 2-letter ISO country code");
    }
    char clean_geo[4] = {0};
    if (opts.geo_country && opts.geo_country[0]) {
        sanitize_country_code(opts.geo_country, clean_geo, sizeof(clean_geo));
        if (strcmp(clean_geo, "XX") == 0) {
            return send_error(connection, MHD_HTTP_BAD_REQUEST, "geo_country must be a valid 2-letter ISO country code");
        }
        opts.geo_country = clean_geo;
    }
    if (get_optional_string(json, "geo_url", MAX_URL_LEN, &opts.geo_url) != 0 ||
        (opts.geo_url && !is_safe_redirect_url(opts.geo_url, g_base_url, MAX_URL_LEN))) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "geo_url must be a valid public HTTP or HTTPS URL");
    }
    const cJSON *inter_item = cJSON_GetObjectItemCaseSensitive(json, "interstitial");
    opts.interstitial = (inter_item && (cJSON_IsTrue(inter_item) || (cJSON_IsNumber(inter_item) && inter_item->valueint != 0))) ? 1 : 0;

    char slug[MAX_SLUG_LEN];
    int custom_slug_requested = cJSON_IsString(custom_slug) && custom_slug->valuestring &&
                                custom_slug->valuestring[0] != '\0';
    if (custom_slug_requested) {
        size_t input_len = strnlen(custom_slug->valuestring, 129U);
        if (input_len > 128U) {
            return send_error(connection, MHD_HTTP_BAD_REQUEST, "Custom slug is too long");
        }
        slugify(custom_slug->valuestring, slug, (int)sizeof(slug));
        if (!is_valid_slug(slug, MAX_SLUG_LENGTH) || is_reserved_slug(slug)) {
            return send_error(connection, MHD_HTTP_BAD_REQUEST, "Custom slug is invalid or reserved");
        }
    }

    int insert_result = -1;
    int attempts = custom_slug_requested ? 1 : 5;
    for (int attempt = 0; attempt < attempts; attempt++) {
        if (!custom_slug_requested && generate_random_slug(slug, 8U) != 0) {
            log_error("Secure random generator failed while creating a slug");
            return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Could not allocate a secure slug");
        }
        insert_result = insert_link_ex(slug, final_url, &opts);
        if (insert_result == 0) break;
    }
    if (insert_result != 0) {
        return send_error(connection, MHD_HTTP_CONFLICT,
                          custom_slug_requested ? "Custom slug already exists" : "Could not allocate a unique slug");
    }

    (void)record_audit_event("link.create", actor, slug, client_ip, final_url);
    log_message("Created short link: %s (by %s)", slug, actor);
    return send_short_link_created_json(connection, MHD_HTTP_CREATED, slug, 0);
}

static int handle_bulk_shorten(struct MHD_Connection *connection, const char *client_ip,
                               cJSON *json) {
    char actor[64];
    if (!check_create_permission(connection, actor, sizeof(actor))) {
        return send_error(connection, MHD_HTTP_UNAUTHORIZED, "Invalid or missing API key");
    }
    if (scoped_rate_limit("create", client_ip, CREATE_RATE_LIMIT) != 0) {
        return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
    }

    cJSON *items = cJSON_GetObjectItemCaseSensitive(json, "links");
    int count = cJSON_IsArray(items) ? cJSON_GetArraySize(items) : 0;
    if (count < 1 || count > 25) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "Provide a 'links' array with 1 to 25 items");
    }

    cJSON *res_root = cJSON_CreateObject();
    cJSON *created_arr = cJSON_CreateArray();
    if (!res_root || !created_arr) {
        cJSON_Delete(res_root);
        cJSON_Delete(created_arr);
        return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error");
    }

    cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, items) {
        if (!cJSON_IsObject(entry)) continue;
        cJSON *u = cJSON_GetObjectItemCaseSensitive(entry, "url");
        if (!cJSON_IsString(u) || !u->valuestring ||
            !is_safe_redirect_url(u->valuestring, g_base_url, MAX_URL_LEN)) {
            continue;
        }
        struct link_options opts;
        memset(&opts, 0, sizeof(opts));
        (void)get_optional_string(entry, "title", MAX_TITLE_LEN, &opts.title);
        if (get_optional_string(entry, "tags", MAX_TAGS_LEN, &opts.tags) != 0 ||
            !is_valid_tag_list(opts.tags, MAX_TAGS_LEN)) {
            opts.tags = NULL;
        }
        (void)get_optional_int_range(entry, "ttl_hours", 0, MAX_TTL_HOURS, 0, &opts.ttl_hours);
        (void)get_optional_int_range(entry, "max_clicks", 0, MAX_CLICKS_LIMIT, 0, &opts.max_clicks);

        char slug[MAX_SLUG_LEN];
        int ok = -1;
        cJSON *cslug = cJSON_GetObjectItemCaseSensitive(entry, "custom_slug");
        if (cJSON_IsString(cslug) && cslug->valuestring && cslug->valuestring[0]) {
            slugify(cslug->valuestring, slug, (int)sizeof(slug));
            if (is_valid_slug(slug, MAX_SLUG_LENGTH) && !is_reserved_slug(slug)) {
                ok = insert_link_ex(slug, u->valuestring, &opts);
            }
        } else {
            for (int a = 0; a < 5; a++) {
                if (generate_random_slug(slug, 8U) == 0 &&
                    insert_link_ex(slug, u->valuestring, &opts) == 0) {
                    ok = 0;
                    break;
                }
            }
        }
        if (ok == 0) {
            char short_url[MAX_FULL_URL_LEN];
            char qr_url[MAX_FULL_URL_LEN];
            snprintf(short_url, sizeof(short_url), "%s/%s", g_base_url, slug);
            snprintf(qr_url, sizeof(qr_url), "%s/qr/%s.svg", g_base_url, slug);
            cJSON *item_out = cJSON_CreateObject();
            if (item_out) {
                cJSON_AddStringToObject(item_out, "slug", slug);
                cJSON_AddStringToObject(item_out, "url", u->valuestring);
                cJSON_AddStringToObject(item_out, "short_url", short_url);
                cJSON_AddStringToObject(item_out, "qr_url", qr_url);
                cJSON_AddItemToArray(created_arr, item_out);
            }
        }
    }

    int created_count = cJSON_GetArraySize(created_arr);
    cJSON_AddNumberToObject(res_root, "created", created_count);
    cJSON_AddItemToObject(res_root, "results", created_arr);
    char *out_str = cJSON_PrintUnformatted(res_root);
    cJSON_Delete(res_root);
    if (!out_str) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error");
    (void)record_audit_event("link.bulk_create", actor, "batch", client_ip, "Bulk link creation");
    int ret = send_json_response(connection, MHD_HTTP_CREATED, out_str);
    free(out_str);
    return ret;
}

static int handle_unlock(struct MHD_Connection *connection, const char *client_ip,
                         const char *url, cJSON *json) {
    const char *slug = url + 8;
    if (!is_valid_slug(slug, MAX_SLUG_LENGTH)) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid slug");
    }

    char rate_key[INET6_ADDRSTRLEN + MAX_SLUG_LEN + 16U];
    int rate_key_len = snprintf(rate_key, sizeof(rate_key), "unlock:%s:%s", client_ip, slug);
    if (rate_key_len < 0 || (size_t)rate_key_len >= sizeof(rate_key) ||
        check_rate_limit(rate_key, UNLOCK_RATE_LIMIT) != 0) {
        return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
    }

    cJSON *password = cJSON_GetObjectItemCaseSensitive(json, "password");
    if (!cJSON_IsString(password) || !password->valuestring ||
        strnlen(password->valuestring, MAX_PASSWORD_LEN + 1U) > MAX_PASSWORD_LEN) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "Password is invalid");
    }

    char target_url[MAX_URL_BUFFER];
    int db_result = get_link_with_password(slug, password->valuestring,
                                           target_url, sizeof(target_url));
    if (db_result == -2 || db_result == -3) return send_error(connection, MHD_HTTP_GONE, "Link expired or click limit reached");
    if (db_result == -5) return send_error(connection, MHD_HTTP_FORBIDDEN, "Link is not active yet");
    if (db_result == -6) return send_error(connection, MHD_HTTP_GONE, "Link is disabled");
    if (db_result == -4) {
        (void)record_audit_event("link.unlock_failed", "visitor", slug, client_ip, "Wrong password");
        return send_error(connection, MHD_HTTP_UNAUTHORIZED, "Wrong password");
    }
    if (db_result != 0 || !is_safe_redirect_url(target_url, g_base_url, MAX_URL_LEN)) {
        return send_error(connection, MHD_HTTP_NOT_FOUND, "Not found");
    }

    // Record visit on successful unlock so password-protected links track visits and enforce max_clicks
    record_request_visit(connection, slug, client_ip);

    cJSON *response_json = cJSON_CreateObject();
    if (!response_json || !cJSON_AddStringToObject(response_json, "url", target_url)) {
        cJSON_Delete(response_json);
        return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error");
    }
    char *response_text = cJSON_PrintUnformatted(response_json);
    cJSON_Delete(response_json);
    if (!response_text) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error");

    int ret = send_json_response(connection, MHD_HTTP_OK, response_text);
    free(response_text);
    return ret;
}

static int handle_admin_update_link(struct MHD_Connection *connection, const char *client_ip,
                                    const char *slug, cJSON *json) {
    if (!check_admin_api_key(connection)) {
        (void)record_audit_event("auth.failed", "anonymous", slug, client_ip, "Admin update unauthorized");
        return send_error(connection, MHD_HTTP_UNAUTHORIZED, "Invalid or missing API key");
    }
    if (scoped_rate_limit("admin", client_ip, ADMIN_RATE_LIMIT) != 0) {
        return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
    }
    if (!is_valid_slug(slug, MAX_SLUG_LENGTH) || is_reserved_slug(slug)) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid slug");
    }

    cJSON *target_url = cJSON_GetObjectItemCaseSensitive(json, "url");
    if (!cJSON_IsString(target_url) || !target_url->valuestring ||
        !is_safe_redirect_url(target_url->valuestring, g_base_url, MAX_URL_LEN)) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "A valid public HTTP or HTTPS URL is required");
    }

    const char *title = NULL;
    const char *tags = NULL;
    if (get_optional_string(json, "title", MAX_TITLE_LEN, &title) != 0) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid title");
    }
    if (get_optional_string(json, "tags", MAX_TAGS_LEN, &tags) != 0 ||
        !is_valid_tag_list(tags, MAX_TAGS_LEN)) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid tags");
    }

    int ttl_hours = -1;
    int max_clicks = 0;
    if (get_optional_int_range(json, "ttl_hours", -1, MAX_TTL_HOURS, -1, &ttl_hours) != 0 ||
        get_optional_int_range(json, "max_clicks", 0, MAX_CLICKS_LIMIT, 0, &max_clicks) != 0) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid ttl_hours or max_clicks");
    }

    int is_active = 1;
    cJSON *active_item = cJSON_GetObjectItemCaseSensitive(json, "is_active");
    if (active_item) {
        if (cJSON_IsBool(active_item)) is_active = cJSON_IsTrue(active_item) ? 1 : 0;
        else if (cJSON_IsNumber(active_item)) is_active = active_item->valueint ? 1 : 0;
        else return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid is_active flag");
    }

    const char *new_password = NULL;
    cJSON *pwd_item = cJSON_GetObjectItemCaseSensitive(json, "password");
    if (pwd_item && !cJSON_IsNull(pwd_item)) {
        if (!cJSON_IsString(pwd_item) || !pwd_item->valuestring ||
            strnlen(pwd_item->valuestring, MAX_PASSWORD_LEN + 1U) > MAX_PASSWORD_LEN) {
            return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid password");
        }
        new_password = pwd_item->valuestring;
    }

    if (update_link(slug, target_url->valuestring, title, tags, ttl_hours,
                    max_clicks, is_active, new_password) != 0) {
        return send_error(connection, MHD_HTTP_NOT_FOUND, "Link not found");
    }

    (void)record_audit_event("link.update", "admin", slug, client_ip, target_url->valuestring);
    return send_json_response(connection, MHD_HTTP_OK, "{\"updated\":true}");
}

static int handle_admin_create_token(struct MHD_Connection *connection, const char *client_ip,
                                     cJSON *json) {
    if (!check_admin_api_key(connection)) {
        return send_error(connection, MHD_HTTP_UNAUTHORIZED, "Invalid or missing API key");
    }
    if (scoped_rate_limit("admin", client_ip, ADMIN_RATE_LIMIT) != 0) {
        return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
    }

    const char *name = NULL;
    int ttl_days = 0;
    if (get_optional_string(json, "name", 64U, &name) != 0 || !name ||
        get_optional_int_range(json, "ttl_days", 0, 3650, 0, &ttl_days) != 0) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "Valid token name and optional ttl_days required");
    }

    char raw_token[64];
    if (create_api_token(name, ttl_days, raw_token, sizeof(raw_token)) != 0) {
        return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Could not create token");
    }

    cJSON *res = cJSON_CreateObject();
    if (!res || !cJSON_AddStringToObject(res, "name", name) ||
        !cJSON_AddStringToObject(res, "token", raw_token)) {
        cJSON_Delete(res);
        OPENSSL_cleanse(raw_token, sizeof(raw_token));
        return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error");
    }
    OPENSSL_cleanse(raw_token, sizeof(raw_token));
    char *text = cJSON_PrintUnformatted(res);
    cJSON_Delete(res);
    if (!text) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error");

    (void)record_audit_event("token.create", "admin", name, client_ip, "Created scoped creator token");
    int ret = send_json_response(connection, MHD_HTTP_CREATED, text);
    free(text);
    return ret;
}

static int handle_admin_cleanup(struct MHD_Connection *connection, const char *client_ip) {
    if (!check_admin_api_key(connection)) {
        return send_error(connection, MHD_HTTP_UNAUTHORIZED, "Invalid or missing API key");
    }
    if (scoped_rate_limit("admin", client_ip, ADMIN_RATE_LIMIT) != 0) {
        return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
    }

    int expired = cleanup_expired_links();
    if (expired < 0) expired = 0;

    char buf[64];
    snprintf(buf, sizeof(buf), "{\"expired_removed\":%d}", expired);
    (void)record_audit_event("admin.cleanup", "admin", NULL, client_ip, buf);
    return send_json_response(connection, MHD_HTTP_OK, buf);
}

static int handle_admin_bulk_action(struct MHD_Connection *connection, const char *client_ip,
                                    cJSON *json) {
    if (!check_admin_api_key(connection)) {
        return send_error(connection, MHD_HTTP_UNAUTHORIZED, "Invalid or missing API key");
    }
    if (scoped_rate_limit("admin", client_ip, ADMIN_RATE_LIMIT) != 0) {
        return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
    }

    const char *action = NULL;
    if (get_optional_string(json, "action", 16U, &action) != 0 || !action) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "action must be enable, disable, or delete");
    }

    cJSON *slugs_arr = cJSON_GetObjectItemCaseSensitive(json, "slugs");
    int count = cJSON_IsArray(slugs_arr) ? cJSON_GetArraySize(slugs_arr) : 0;
    if (count < 1 || count > 50) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "Provide a 'slugs' array with 1 to 50 items");
    }

    const char *slug_list[50];
    int valid_count = 0;
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, slugs_arr) {
        if (cJSON_IsString(item) && item->valuestring &&
            is_valid_slug(item->valuestring, MAX_SLUG_LENGTH)) {
            slug_list[valid_count++] = item->valuestring;
        }
    }
    if (valid_count == 0) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "No valid slugs provided");
    }

    int affected = bulk_admin_action(action, slug_list, valid_count);
    if (affected < 0) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid bulk action");
    }

    char out[96];
    snprintf(out, sizeof(out), "{\"status\":\"ok\",\"action\":\"%s\",\"affected\":%d}", action, affected);
    (void)record_audit_event("link.bulk_action", "admin", action, client_ip, out);
    return send_json_response(connection, MHD_HTTP_OK, out);
}

static int handle_admin_restore(struct MHD_Connection *connection, const char *client_ip,
                                const cJSON *json) {
    if (!check_admin_api_key(connection)) {
        return send_error(connection, MHD_HTTP_UNAUTHORIZED, "Invalid or missing API key");
    }
    if (scoped_rate_limit("admin", client_ip, ADMIN_RATE_LIMIT) != 0) {
        return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
    }

    int restored = 0;
    int skipped = 0;
    if (import_links_backup_json(json, g_base_url, &restored, &skipped) != 0) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid backup snapshot JSON");
    }

    char out[128];
    snprintf(out, sizeof(out), "{\"status\":\"ok\",\"restored\":%d,\"skipped\":%d}", restored, skipped);
    (void)record_audit_event("admin.restore_json", "admin", NULL, client_ip, out);
    return send_json_response(connection, MHD_HTTP_OK, out);
}

static int handle_admin_maintenance(struct MHD_Connection *connection, const char *client_ip) {
    if (!check_admin_api_key(connection)) {
        return send_error(connection, MHD_HTTP_UNAUTHORIZED, "Invalid or missing API key");
    }
    if (scoped_rate_limit("admin", client_ip, ADMIN_RATE_LIMIT) != 0) {
        return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
    }

    char *report = run_db_maintenance_json();
    if (!report) {
        return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Database maintenance failed");
    }
    (void)record_audit_event("admin.maintenance", "admin", NULL, client_ip, "Optimized SQLite & WAL checkpoint");
    int ret = send_json_response(connection, MHD_HTTP_OK, report);
    free(report);
    return ret;
}

static int handle_inspect_page(struct MHD_Connection *connection, const char *slug,
                               const char *client_ip) {
    if (!is_valid_slug(slug, MAX_SLUG_LENGTH)) {
        return send_error_page(connection, MHD_HTTP_BAD_REQUEST, "Bad Request", "The link identifier is invalid.");
    }
    if (scoped_rate_limit("read", client_ip, STATS_RATE_LIMIT) != 0) {
        return send_error_page(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Too Many Requests", "Please wait a moment and try again.");
    }

    char *json_str = get_link_inspect_json(slug);
    if (!json_str) {
        return send_error_page(connection, MHD_HTTP_NOT_FOUND, "Not Found", "This short link does not exist.");
    }

    cJSON *info = cJSON_Parse(json_str);
    free(json_str);
    if (!info) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error");

    const char *status = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(info, "status"));
    const char *title = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(info, "title"));
    const char *target = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(info, "target_url"));
    const char *created = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(info, "created_at"));
    const char *expires = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(info, "expires_at"));
    int has_pwd = cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(info, "has_password"));
    cJSON *visits_item = cJSON_GetObjectItemCaseSensitive(info, "total_visits");
    int visits = visits_item ? visits_item->valueint : 0;

    char esc_status[64], esc_title[256], esc_target[2200], esc_created[64], esc_expires[64];
    html_escape(status ? status : "active", esc_status, sizeof(esc_status));
    html_escape(title ? title : "-", esc_title, sizeof(esc_title));
    html_escape(has_pwd ? "Protected (Requires Password)" : (target ? target : "Unavailable"),
                esc_target, sizeof(esc_target));
    html_escape(created ? created : "-", esc_created, sizeof(esc_created));
    html_escape(expires ? expires : "Never", esc_expires, sizeof(esc_expires));
    cJSON_Delete(info);

    const char *pwd_label = has_pwd ? "Yes (PBKDF2-SHA256)" : "No";
    int html_len = snprintf(NULL, 0, INSPECT_HTML_TEMPLATE,
                            slug, slug, slug, slug,
                            esc_status, esc_title, esc_target, pwd_label,
                            visits, esc_created, esc_expires,
                            slug, slug, slug);
    if (html_len < 0) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Rendering error");

    char *html = malloc((size_t)html_len + 1U);
    if (!html) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error");
    snprintf(html, (size_t)html_len + 1U, INSPECT_HTML_TEMPLATE,
             slug, slug, slug, slug,
             esc_status, esc_title, esc_target, pwd_label,
             visits, esc_created, esc_expires,
             slug, slug, slug);
    return send_html_dynamic(connection, MHD_HTTP_OK, html);
}

static int handle_social_preview_page(struct MHD_Connection *connection, const char *slug,
                                      const char *client_ip) {
    if (!is_valid_slug(slug, MAX_SLUG_LENGTH)) {
        return send_error_page(connection, MHD_HTTP_BAD_REQUEST, "Bad Request", "The link identifier is invalid.");
    }
    if (scoped_rate_limit("read", client_ip, STATS_RATE_LIMIT) != 0) {
        return send_error_page(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Too Many Requests", "Please wait a moment.");
    }

    char *json_str = get_link_inspect_json(slug);
    if (!json_str) {
        return send_error_page(connection, MHD_HTTP_NOT_FOUND, "Not Found", "This short link does not exist.");
    }

    cJSON *info = cJSON_Parse(json_str);
    free(json_str);
    if (!info) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error");

    const char *title = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(info, "title"));
    const char *domain = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(info, "target_domain"));
    cJSON *visits_item = cJSON_GetObjectItemCaseSensitive(info, "total_visits");
    int visits = visits_item ? visits_item->valueint : 0;

    char esc_title[256], esc_domain[256];
    html_escape(title && title[0] ? title : slug, esc_title, sizeof(esc_title));
    html_escape(domain && domain[0] ? domain : "protected destination", esc_domain, sizeof(esc_domain));
    cJSON_Delete(info);

    int html_len = snprintf(NULL, 0, SOCIAL_PREVIEW_HTML_TEMPLATE,
                            esc_title, esc_domain, visits, g_base_url, slug, g_base_url, slug,
                            esc_title, esc_domain, esc_title, esc_title, esc_domain,
                            slug, slug, slug, slug, visits);
    if (html_len < 0) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Rendering error");

    char *html = malloc((size_t)html_len + 1U);
    if (!html) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error");
    snprintf(html, (size_t)html_len + 1U, SOCIAL_PREVIEW_HTML_TEMPLATE,
             esc_title, esc_domain, visits, g_base_url, slug, g_base_url, slug,
             esc_title, esc_domain, esc_title, esc_title, esc_domain,
             slug, slug, slug, slug, visits);
    return send_html_dynamic(connection, MHD_HTTP_OK, html);
}

static int handle_bio_page(struct MHD_Connection *connection, const char *profile,
                           const char *client_ip) {
    if (!is_valid_slug(profile, 24U)) {
        return send_error_page(connection, MHD_HTTP_BAD_REQUEST, "Bad Request", "Invalid profile handle.");
    }
    if (scoped_rate_limit("read", client_ip, STATS_RATE_LIMIT) != 0) {
        return send_error_page(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Too Many Requests", "Please wait a moment.");
    }

    char *json_str = get_bio_collection_json(profile);
    if (!json_str) {
        return send_error_page(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Error", "Could not load bio profile.");
    }

    cJSON *root = cJSON_Parse(json_str);
    free(json_str);
    if (!root) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error");

    size_t cap = 8192U;
    char *html = malloc(cap);
    if (!html) {
        cJSON_Delete(root);
        return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error");
    }

    int head_len = snprintf(html, cap, BIO_HTML_HEADER, profile, profile, profile, profile);
    size_t pos = head_len > 0 ? (size_t)head_len : 0;

    cJSON *links = cJSON_GetObjectItemCaseSensitive(root, "links");
    int count = cJSON_IsArray(links) ? cJSON_GetArraySize(links) : 0;
    if (count == 0) {
        int n = snprintf(html + pos, cap - pos,
                         "<div class=\"panel\"><p class=\"hint\">No public links tagged with <code>%s</code> yet.</p></div>\n",
                         profile);
        if (n > 0) pos += (size_t)n;
    } else {
        cJSON *item = NULL;
        cJSON_ArrayForEach(item, links) {
            const char *slug = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(item, "slug"));
            const char *title = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(item, "title"));
            cJSON *clicks_item = cJSON_GetObjectItemCaseSensitive(item, "clicks");
            int clicks = clicks_item ? clicks_item->valueint : 0;
            if (!slug) continue;

            char esc_title[256];
            html_escape(title && title[0] ? title : slug, esc_title, sizeof(esc_title));
            if (pos + 512U >= cap) break;
            int n = snprintf(html + pos, cap - pos,
                             "<div class=\"field\"><a class=\"btn primary\" href=\"/%s\" rel=\"noopener noreferrer\">"
                             "%s <span class=\"badge tag\">%d visits</span></a></div>\n",
                             slug, esc_title, clicks);
            if (n > 0) pos += (size_t)n;
        }
    }

    cJSON_Delete(root);
    snprintf(html + pos, cap - pos, "%s", BIO_HTML_FOOTER);
    return send_html_dynamic(connection, MHD_HTTP_OK, html);
}

static int handle_qr_request(struct MHD_Connection *connection, const char *raw_slug,
                             const char *client_ip) {
    char slug[MAX_SLUG_LEN];
    size_t raw_len = strnlen(raw_slug, MAX_SLUG_LEN + 8U);
    if (raw_len == 0 || raw_len >= MAX_SLUG_LEN + 5U) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid slug");
    }

    if (raw_len > 4U && strcmp(raw_slug + raw_len - 4U, ".svg") == 0) {
        raw_len -= 4U;
    }
    if (raw_len >= sizeof(slug)) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid slug");
    }
    memcpy(slug, raw_slug, raw_len);
    slug[raw_len] = '\0';

    if (!is_valid_slug(slug, MAX_SLUG_LENGTH)) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid slug");
    }
    if (scoped_rate_limit("read", client_ip, STATS_RATE_LIMIT) != 0) {
        return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
    }

    char ignored_url[MAX_URL_BUFFER];
    int requires_pwd = 0;
    int db_res = get_link(slug, ignored_url, sizeof(ignored_url), &requires_pwd);
    if (db_res == -1) {
        return send_error(connection, MHD_HTTP_NOT_FOUND, "Link not found");
    }

    char full_url[MAX_FULL_URL_LEN];
    int len = snprintf(full_url, sizeof(full_url), "%s/%s", g_base_url, slug);
    if (len < 0 || (size_t)len >= sizeof(full_url)) {
        return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Configuration error");
    }

    const char *theme = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "theme");
    const char *fg_param = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "fg");
    const char *bg_param = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "bg");
    const char *border_param = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "border");

    const char *fg_hex = "#0f172a";
    const char *bg_hex = "#ffffff";
    int border = 4;

    if (theme) {
        if (strcmp(theme, "dark") == 0) { fg_hex = "#e2e8f0"; bg_hex = "#0f172a"; }
        else if (strcmp(theme, "cyber") == 0) { fg_hex = "#38bdf8"; bg_hex = "#0b1120"; }
        else if (strcmp(theme, "emerald") == 0) { fg_hex = "#064e3b"; bg_hex = "#ecfdf5"; }
        else if (strcmp(theme, "sunset") == 0) { fg_hex = "#7c2d12"; bg_hex = "#fff7ed"; }
    }

    char custom_fg[8], custom_bg[8];
    if (fg_param && strlen(fg_param) == 6U) {
        snprintf(custom_fg, sizeof(custom_fg), "#%s", fg_param);
        fg_hex = custom_fg;
    }
    if (bg_param && strlen(bg_param) == 6U) {
        snprintf(custom_bg, sizeof(custom_bg), "#%s", bg_param);
        bg_hex = custom_bg;
    }
    if (border_param && border_param[0]) {
        int b = atoi(border_param);
        if (b >= 1 && b <= 12) border = b;
    }

    char *svg = qr_generate_svg_themed(full_url, border, fg_hex, bg_hex);
    if (!svg) {
        return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Could not generate QR SVG");
    }
    return send_dynamic_buffer(connection, MHD_HTTP_OK, svg, "image/svg+xml; charset=utf-8", NULL);
}

static int handle_badge_request(struct MHD_Connection *connection, const char *raw_slug,
                                const char *client_ip) {
    char slug[MAX_SLUG_LEN];
    size_t raw_len = strnlen(raw_slug, MAX_SLUG_LEN + 8U);
    if (raw_len == 0 || raw_len >= MAX_SLUG_LEN + 5U) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid slug");
    }
    if (raw_len > 4U && strcmp(raw_slug + raw_len - 4U, ".svg") == 0) {
        raw_len -= 4U;
    }
    if (raw_len >= sizeof(slug)) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid slug");
    }
    memcpy(slug, raw_slug, raw_len);
    slug[raw_len] = '\0';

    if (!is_valid_slug(slug, MAX_SLUG_LENGTH)) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid slug");
    }
    if (scoped_rate_limit("read", client_ip, STATS_RATE_LIMIT) != 0) {
        return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
    }

    char ignored_url[MAX_URL_BUFFER];
    int requires_pwd = 0;
    int db_res = get_link(slug, ignored_url, sizeof(ignored_url), &requires_pwd);
    if (db_res == -1) {
        return send_error(connection, MHD_HTTP_NOT_FOUND, "Link not found");
    }

    int total = 0;
    int unique = 0;
    (void)get_stats(slug, &total, &unique);

    const char *label_param = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "label");
    const char *color_param = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "color");
    const char *metric_param = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "metric");

    char esc_label[48];
    html_escape((label_param && label_param[0]) ? label_param : slug, esc_label, sizeof(esc_label));

    const char *accent = "#0284c7";
    if (color_param) {
        if (strcmp(color_param, "emerald") == 0) accent = "#059669";
        else if (strcmp(color_param, "amber") == 0) accent = "#d97706";
        else if (strcmp(color_param, "rose") == 0) accent = "#e11d48";
        else if (strcmp(color_param, "violet") == 0) accent = "#7c3aed";
    }

    char value_buf[48];
    if (metric_param && strcmp(metric_param, "unique") == 0) {
        snprintf(value_buf, sizeof(value_buf), "%d unique", unique);
    } else if (metric_param && strcmp(metric_param, "status") == 0) {
        snprintf(value_buf, sizeof(value_buf), "%s", (db_res == 0) ? "active" : "inactive");
    } else {
        snprintf(value_buf, sizeof(value_buf), "%d visits", total);
    }

    int left_w = (int)(strlen(esc_label) * 7U + 18U);
    if (left_w < 54) left_w = 54;
    if (left_w > 180) left_w = 180;
    int right_w = (int)(strlen(value_buf) * 7U + 18U);
    if (right_w < 64) right_w = 64;
    int total_w = left_w + right_w;

    char *svg = malloc(1536U);
    if (!svg) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error");
    snprintf(svg, 1536U,
             "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%d\" height=\"20\" role=\"img\" aria-label=\"%s: %s\">"
             "<linearGradient id=\"s\" x2=\"0\" y2=\"100%%\">"
             "<stop offset=\"0\" stop-color=\"#fff\" stop-opacity=\".12\"/>"
             "<stop offset=\"1\" stop-opacity=\".12\"/>"
             "</linearGradient>"
             "<clipPath id=\"r\"><rect width=\"%d\" height=\"20\" rx=\"4\" fill=\"#fff\"/></clipPath>"
             "<g clip-path=\"url(#r)\">"
             "<rect width=\"%d\" height=\"20\" fill=\"#0f172a\"/>"
             "<rect x=\"%d\" width=\"%d\" height=\"20\" fill=\"%s\"/>"
             "<rect width=\"%d\" height=\"20\" fill=\"url(#s)\"/>"
             "</g>"
             "<g fill=\"#fff\" text-anchor=\"middle\" font-family=\"ui-monospace,SFMono-Regular,Menlo,Monaco,Consolas,monospace\" text-rendering=\"geometricPrecision\" font-size=\"11\">"
             "<text x=\"%d\" y=\"14\">%s</text>"
             "<text x=\"%d\" y=\"14\" font-weight=\"bold\">%s</text>"
             "</g>"
             "</svg>",
             total_w, esc_label, value_buf,
             total_w, left_w, left_w, right_w, accent, total_w,
             left_w / 2, esc_label, left_w + (right_w / 2), value_buf);
    return send_dynamic_buffer(connection, MHD_HTTP_OK, svg, "image/svg+xml; charset=utf-8", NULL);
}

static int handle_get(struct MHD_Connection *connection, const char *url, const char *client_ip) {
    if (strcmp(url, "/health") == 0) {
        return send_json_response(connection, MHD_HTTP_OK, "{\"status\":\"ok\"}");
    }

    // Static C-embedded assets (enables strict CSP with zero 'unsafe-inline')
    if (strcmp(url, "/assets/styles.css") == 0) {
        return send_text_response(connection, MHD_HTTP_OK, APP_CSS, "text/css; charset=utf-8");
    }
    if (strcmp(url, "/assets/index.js") == 0) {
        return send_text_response(connection, MHD_HTTP_OK, INDEX_JS, "application/javascript; charset=utf-8");
    }
    if (strcmp(url, "/assets/admin.js") == 0) {
        return send_text_response(connection, MHD_HTTP_OK, ADMIN_JS, "application/javascript; charset=utf-8");
    }
    if (strcmp(url, "/assets/password.js") == 0) {
        return send_text_response(connection, MHD_HTTP_OK, PASSWORD_JS, "application/javascript; charset=utf-8");
    }
    if (strcmp(url, "/assets/stats.js") == 0) {
        return send_text_response(connection, MHD_HTTP_OK, STATS_JS, "application/javascript; charset=utf-8");
    }
    if (strcmp(url, "/assets/docs.js") == 0) {
        return send_text_response(connection, MHD_HTTP_OK, DOCS_JS, "application/javascript; charset=utf-8");
    }
    if (strcmp(url, "/favicon.svg") == 0) {
        return send_text_response(connection, MHD_HTTP_OK, FAVICON_SVG, "image/svg+xml; charset=utf-8");
    }
    if (strcmp(url, "/favicon.ico") == 0) {
        return send_empty_response(connection, MHD_HTTP_NO_CONTENT);
    }
    if (strcmp(url, "/manifest.json") == 0) {
        return send_text_response(connection, MHD_HTTP_OK, MANIFEST_JSON, "application/manifest+json; charset=utf-8");
    }
    if (strcmp(url, "/docs") == 0) {
        return send_html(connection, MHD_HTTP_OK, DOCS_HTML);
    }

    if (strcmp(url, "/openapi.json") == 0) {
        char *spec = malloc(2560U);
        if (!spec) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error");
        snprintf(spec, 2560U,
                 "{\n"
                 "  \"openapi\": \"3.0.3\",\n"
                 "  \"info\": {\n"
                 "    \"title\": \"c.micutu.com C17 URL Shortener API\",\n"
                 "    \"version\": \"2.1.0\",\n"
                 "    \"description\": \"High-performance URL shortener, native SVG QR & badge generator, Geo/A-B router, and privacy-first analytics engine in pure C17.\"\n"
                 "  },\n"
                 "  \"servers\": [{ \"url\": \"%s\" }],\n"
                 "  \"paths\": {\n"
                 "    \"/shorten\": { \"post\": { \"summary\": \"Create a short link (supports UTM, A/B, Mobile & Geo routing)\" } },\n"
                 "    \"/api/bulk/shorten\": { \"post\": { \"summary\": \"Batch shorten up to 25 links\" } },\n"
                 "    \"/qr/{slug}.svg\": { \"get\": { \"summary\": \"Generate themed ISO 18004 SVG QR code in pure C\" } },\n"
                 "    \"/badge/{slug}.svg\": { \"get\": { \"summary\": \"Generate dynamic shields-style SVG analytics badge\" } },\n"
                 "    \"/api/inspect/{slug}\": { \"get\": { \"summary\": \"Inspect link safety, routing policies, and metadata\" } },\n"
                 "    \"/api/stats/{slug}\": { \"get\": { \"summary\": \"Query privacy-preserving link analytics, 24h heatmap, and countries\" } },\n"
                 "    \"/api/stats/{slug}/export.csv\": { \"get\": { \"summary\": \"Export per-link analytics (daily, country, device) as CSV\" } },\n"
                 "    \"/api/bio/{tag}\": { \"get\": { \"summary\": \"Query Link-in-Bio public collection\" } },\n"
                 "    \"/api/admin/links\": { \"get\": { \"summary\": \"Query and filter links\" } },\n"
                 "    \"/api/admin/bulk-action\": { \"post\": { \"summary\": \"Batch enable, disable, reset clicks, or delete links\" } },\n"
                 "    \"/api/admin/backup.json\": { \"get\": { \"summary\": \"Export JSON snapshot backup\" } },\n"
                 "    \"/api/admin/restore\": { \"post\": { \"summary\": \"Restore links from JSON snapshot\" } },\n"
                 "    \"/api/admin/maintenance\": { \"post\": { \"summary\": \"Optimize SQLite indexes, purge expired records, and truncate WAL\" } },\n"
                 "    \"/metrics\": { \"get\": { \"summary\": \"Prometheus metrics exporter\" } },\n"
                 "    \"/health\": { \"get\": { \"summary\": \"Service health check\" } }\n"
                 "  }\n"
                 "}\n",
                 g_base_url);
        return send_dynamic_buffer(connection, MHD_HTTP_OK, spec, "application/json; charset=utf-8", NULL);
    }

    if (strcmp(url, "/robots.txt") == 0) {
        char *robots = malloc(512U);
        if (!robots) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error");
        snprintf(robots, 512U,
                 "User-agent: *\n"
                 "Allow: /\n"
                 "Disallow: /stats/\n"
                 "Disallow: /inspect/\n"
                 "Disallow: /qr/\n"
                 "Disallow: /admin\n"
                 "Disallow: /api/\n"
                 "Sitemap: %s/sitemap.xml\n",
                 g_base_url);
        return send_dynamic_buffer(connection, MHD_HTTP_OK, robots, "text/plain; charset=utf-8", NULL);
    }

    if (strcmp(url, "/sitemap.xml") == 0) {
        char *sitemap = malloc(1024U);
        if (!sitemap) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error");
        snprintf(sitemap, 1024U,
                 "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                 "<urlset xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\">\n"
                 "  <url>\n"
                 "    <loc>%s/</loc>\n"
                 "    <changefreq>weekly</changefreq>\n"
                 "    <priority>1.0</priority>\n"
                 "  </url>\n"
                 "</urlset>\n",
                 g_base_url);
        return send_dynamic_buffer(connection, MHD_HTTP_OK, sitemap, "application/xml; charset=utf-8", NULL);
    }

    // The page itself has no privileged data. The API key is entered in-memory by the user.
    if (strcmp(url, "/admin") == 0) {
        if (MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "key")) {
            return send_error_page(connection, MHD_HTTP_BAD_REQUEST, "Bad Request",
                                   "API keys must not be sent in URLs. Open /admin and enter it in the form.");
        }
        return send_html(connection, MHD_HTTP_OK, ADMIN_HTML);
    }

    if (strcmp(url, "/api/admin/links") == 0) {
        if (!check_admin_api_key(connection)) {
            return send_error(connection, MHD_HTTP_UNAUTHORIZED, "Invalid or missing API key");
        }
        if (scoped_rate_limit("admin", client_ip, ADMIN_RATE_LIMIT) != 0) {
            return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
        }

        const char *q = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "q");
        const char *tag = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "tag");
        const char *status = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "status");
        char *json = query_admin_links_json(q, tag, status, 1000, 0);
        if (!json) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Database error");
        int ret = send_json_response(connection, MHD_HTTP_OK, json);
        free(json);
        return ret;
    }

    if (strcmp(url, "/api/admin/export.csv") == 0) {
        if (!check_admin_api_key(connection)) {
            return send_error(connection, MHD_HTTP_UNAUTHORIZED, "Invalid or missing API key");
        }
        if (scoped_rate_limit("admin", client_ip, ADMIN_RATE_LIMIT) != 0) {
            return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
        }
        char *csv = export_links_csv();
        if (!csv) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Database error");
        (void)record_audit_event("admin.export_csv", "admin", NULL, client_ip, "Exported links CSV");
        return send_dynamic_buffer(connection, MHD_HTTP_OK, csv, "text/csv; charset=utf-8",
                                   "attachment; filename=\"shortener-links.csv\"");
    }

    if (strcmp(url, "/api/admin/backup.json") == 0) {
        if (!check_admin_api_key(connection)) {
            return send_error(connection, MHD_HTTP_UNAUTHORIZED, "Invalid or missing API key");
        }
        if (scoped_rate_limit("admin", client_ip, ADMIN_RATE_LIMIT) != 0) {
            return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
        }
        char *backup = export_links_backup_json();
        if (!backup) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Database error");
        (void)record_audit_event("admin.backup_json", "admin", NULL, client_ip, "Exported JSON snapshot backup");
        return send_dynamic_buffer(connection, MHD_HTTP_OK, backup, "application/json; charset=utf-8",
                                   "attachment; filename=\"shortener-backup.json\"");
    }

    if (strcmp(url, "/api/admin/tokens") == 0) {
        if (!check_admin_api_key(connection)) {
            return send_error(connection, MHD_HTTP_UNAUTHORIZED, "Invalid or missing API key");
        }
        if (scoped_rate_limit("admin", client_ip, ADMIN_RATE_LIMIT) != 0) {
            return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
        }
        char *json = list_api_tokens_json();
        if (!json) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Database error");
        int ret = send_json_response(connection, MHD_HTTP_OK, json);
        free(json);
        return ret;
    }

    if (strcmp(url, "/api/admin/audit") == 0) {
        if (!check_admin_api_key(connection)) {
            return send_error(connection, MHD_HTTP_UNAUTHORIZED, "Invalid or missing API key");
        }
        if (scoped_rate_limit("admin", client_ip, ADMIN_RATE_LIMIT) != 0) {
            return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
        }
        char *json = get_audit_logs_json(100);
        if (!json) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Database error");
        int ret = send_json_response(connection, MHD_HTTP_OK, json);
        free(json);
        return ret;
    }

    if (strcmp(url, "/api/admin/system") == 0) {
        if (!check_admin_api_key(connection)) {
            return send_error(connection, MHD_HTTP_UNAUTHORIZED, "Invalid or missing API key");
        }
        if (scoped_rate_limit("admin", client_ip, ADMIN_RATE_LIMIT) != 0) {
            return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
        }
        char *json = get_system_diagnostics_json(get_uptime_seconds());
        if (!json) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Database error");
        int ret = send_json_response(connection, MHD_HTTP_OK, json);
        free(json);
        return ret;
    }

    if (strcmp(url, "/metrics") == 0) {
        if (!check_admin_api_key(connection)) {
            return send_error(connection, MHD_HTTP_UNAUTHORIZED, "Invalid or missing API key");
        }
        if (scoped_rate_limit("admin", client_ip, ADMIN_RATE_LIMIT) != 0) {
            return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
        }
        char *prom = get_prometheus_metrics(get_uptime_seconds());
        if (!prom) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Metrics error");
        return send_dynamic_buffer(connection, MHD_HTTP_OK, prom,
                                   "text/plain; version=0.0.4; charset=utf-8", NULL);
    }

    if (strncmp(url, "/api/bio/", 9) == 0) {
        const char *profile = url + 9;
        if (!is_valid_slug(profile, 24U)) {
            return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid profile handle");
        }
        if (scoped_rate_limit("read", client_ip, STATS_RATE_LIMIT) != 0) {
            return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
        }
        char *json = get_bio_collection_json(profile);
        if (!json) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Database error");
        int ret = send_json_response(connection, MHD_HTTP_OK, json);
        free(json);
        return ret;
    }

    if (strncmp(url, "/bio/", 5) == 0) {
        return handle_bio_page(connection, url + 5, client_ip);
    }

    if (strncmp(url, "/p/", 3) == 0) {
        return handle_social_preview_page(connection, url + 3, client_ip);
    }

    if (strncmp(url, "/api/stats/", 11) == 0) {
        const char *slug = url + 11;
        size_t s_len = strlen(slug);
        if (s_len > 11U && strcmp(slug + s_len - 11U, "/export.csv") == 0) {
            char clean_slug[MAX_SLUG_LEN];
            size_t base_len = s_len - 11U;
            if (base_len >= sizeof(clean_slug)) {
                return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid slug");
            }
            memcpy(clean_slug, slug, base_len);
            clean_slug[base_len] = '\0';
            if (!is_valid_slug(clean_slug, MAX_SLUG_LENGTH)) {
                return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid slug");
            }
            if (scoped_rate_limit("read", client_ip, STATS_RATE_LIMIT) != 0) {
                return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
            }
            char *csv = export_slug_analytics_csv(clean_slug);
            if (!csv) return send_error(connection, MHD_HTTP_NOT_FOUND, "Link not found");
            char disp[96];
            snprintf(disp, sizeof(disp), "attachment; filename=\"analytics-%s.csv\"", clean_slug);
            return send_dynamic_buffer(connection, MHD_HTTP_OK, csv, "text/csv; charset=utf-8", disp);
        }
        if (!is_valid_slug(slug, MAX_SLUG_LENGTH)) {
            return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid slug");
        }
        if (scoped_rate_limit("read", client_ip, STATS_RATE_LIMIT) != 0) {
            return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
        }
        char *json = get_detailed_stats_json(slug);
        if (!json) return send_error(connection, MHD_HTTP_NOT_FOUND, "Link not found");
        int ret = send_json_response(connection, MHD_HTTP_OK, json);
        free(json);
        return ret;
    }

    if (strncmp(url, "/api/inspect/", 13) == 0) {
        const char *slug = url + 13;
        if (!is_valid_slug(slug, MAX_SLUG_LENGTH)) {
            return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid slug");
        }
        if (scoped_rate_limit("read", client_ip, STATS_RATE_LIMIT) != 0) {
            return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
        }
        char *json = get_link_inspect_json(slug);
        if (!json) return send_error(connection, MHD_HTTP_NOT_FOUND, "Link not found");
        int ret = send_json_response(connection, MHD_HTTP_OK, json);
        free(json);
        return ret;
    }

    if (strncmp(url, "/qr/", 4) == 0) {
        return handle_qr_request(connection, url + 4, client_ip);
    }

    if (strncmp(url, "/badge/", 7) == 0) {
        return handle_badge_request(connection, url + 7, client_ip);
    }

    if (strncmp(url, "/inspect/", 9) == 0) {
        return handle_inspect_page(connection, url + 9, client_ip);
    }

    if (strncmp(url, "/stats/", 7) == 0) {
        const char *slug = url + 7;
        if (!is_valid_slug(slug, MAX_SLUG_LENGTH)) {
            return send_error_page(connection, MHD_HTTP_BAD_REQUEST, "Bad Request", "The link identifier is invalid.");
        }
        if (scoped_rate_limit("read", client_ip, STATS_RATE_LIMIT) != 0) {
            return send_error_page(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Too Many Requests", "Please wait a moment and try again.");
        }

        int requires_password = 0;
        char ignored_url[MAX_URL_BUFFER];
        int db_result = get_link(slug, ignored_url, sizeof(ignored_url), &requires_password);
        if (db_result == -1) return send_error_page(connection, MHD_HTTP_NOT_FOUND, "Not Found", "This short link does not exist.");
        if (db_result == -2 || db_result == -3) return send_error_page(connection, MHD_HTTP_GONE, "Link Expired", "This short link has expired or reached its click limit.");

        int total = 0;
        int unique = 0;
        if (get_stats(slug, &total, &unique) != 0) {
            return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Database error");
        }

        int html_len = snprintf(NULL, 0, STATS_HTML, slug, slug, slug, slug, slug, total, unique, slug);
        if (html_len < 0) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Rendering error");
        char *html = malloc((size_t)html_len + 1U);
        if (!html) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error");
        snprintf(html, (size_t)html_len + 1U, STATS_HTML, slug, slug, slug, slug, slug, total, unique, slug);
        return send_html_dynamic(connection, MHD_HTTP_OK, html);
    }

    if (strcmp(url, "/") == 0) {
        return send_html(connection, MHD_HTTP_OK, INDEX_HTML);
    }

    const char *slug = url + 1;
    size_t slug_raw_len = strnlen(slug, MAX_SLUG_LEN + 2U);
    if (slug_raw_len > 1U && slug_raw_len <= MAX_SLUG_LEN && slug[slug_raw_len - 1U] == '+') {
        char clean_slug[MAX_SLUG_LEN];
        memcpy(clean_slug, slug, slug_raw_len - 1U);
        clean_slug[slug_raw_len - 1U] = '\0';
        return handle_inspect_page(connection, clean_slug, client_ip);
    }

    if (!is_valid_slug(slug, MAX_SLUG_LENGTH)) {
        return send_error_page(connection, MHD_HTTP_NOT_FOUND, "Not Found", "This short link does not exist.");
    }

    char target_url[MAX_URL_BUFFER];
    int requires_password = 0;
    int redirect_code = MHD_HTTP_FOUND;
    const char *ua_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "User-Agent");
    const char *cf_country = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "CF-IPCountry");
    if (!cf_country || !cf_country[0]) {
        cf_country = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "X-Country-Code");
    }
    char visitor_device[16];
    classify_user_agent(ua_header, visitor_device, sizeof(visitor_device));
    int db_result = get_link_for_visitor(slug, visitor_device, cf_country,
                                         target_url, sizeof(target_url),
                                         &requires_password, &redirect_code);
    if (db_result == -2 || db_result == -3) {
        return send_error_page(connection, MHD_HTTP_GONE, "Link Expired",
                               "This short link has expired or reached its maximum click limit.");
    }
    if (db_result == -5) {
        return send_error_page(connection, MHD_HTTP_FORBIDDEN, "Not Active Yet",
                               "This scheduled short link is not active yet.");
    }
    if (db_result == -6) {
        return send_error_page(connection, MHD_HTTP_GONE, "Link Disabled",
                               "This short link has been disabled by the administrator.");
    }
    if (db_result != 0) {
        return send_error_page(connection, MHD_HTTP_NOT_FOUND, "Not Found", "This short link does not exist.");
    }

    if (requires_password) return send_html(connection, MHD_HTTP_OK, PASSWORD_HTML);
    if (!is_safe_redirect_url(target_url, g_base_url, MAX_URL_LEN)) {
        log_error("Refused redirect for unsafe stored URL on slug %s", slug);
        return send_error_page(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Unavailable", "This link cannot be redirected safely.");
    }

    // Interstitial safety preview: show destination first, count the visit only on confirmed continue
    const char *direct_param = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "direct");
    if ((!direct_param || strcmp(direct_param, "1") != 0) && is_interstitial_link(slug)) {
        return handle_inspect_page(connection, slug, client_ip);
    }
    record_request_visit(connection, slug, client_ip);
    log_message("Redirected short link: %s", slug);

    static const char empty[] = "";
    struct MHD_Response *response = MHD_create_response_from_buffer(
        0, (void *)empty, MHD_RESPMEM_PERSISTENT);
    if (!response) return MHD_NO;
    MHD_add_response_header(response, "Location", target_url);
    add_security_headers(response);
    int ret = MHD_queue_response(connection, (unsigned int)redirect_code, response);
    MHD_destroy_response(response);
    return ret;
}

static int handle_delete(struct MHD_Connection *connection, const char *url,
                         const char *client_ip) {
    if (!check_admin_api_key(connection)) {
        (void)record_audit_event("auth.failed", "anonymous", url, client_ip, "Unauthorized DELETE");
        return send_error(connection, MHD_HTTP_UNAUTHORIZED, "Invalid or missing API key");
    }
    if (scoped_rate_limit("admin", client_ip, ADMIN_RATE_LIMIT) != 0) {
        return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
    }

    if (strncmp(url, "/api/admin/tokens/", 18) == 0) {
        const char *id_str = url + 18;
        char *end = NULL;
        errno = 0;
        long token_id = strtol(id_str, &end, 10);
        if (errno != 0 || end == id_str || *end != '\0' || token_id < 1 || token_id > 1000000) {
            return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid token ID");
        }
        if (revoke_api_token((int)token_id) != 0) {
            return send_error(connection, MHD_HTTP_NOT_FOUND, "Token not found");
        }
        (void)record_audit_event("token.revoke", "admin", id_str, client_ip, "Revoked scoped creator token");
        return send_json_response(connection, MHD_HTTP_OK, "{\"revoked\":true}");
    }

    const char *slug = url + 1;
    if (!is_valid_slug(slug, MAX_SLUG_LENGTH) || is_reserved_slug(slug)) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid slug");
    }

    if (delete_link(slug) != 0) return send_error(connection, MHD_HTTP_NOT_FOUND, "Slug not found");

    (void)record_audit_event("link.delete", "admin", slug, client_ip, "Deleted link");
    log_message("Deleted short link: %s", slug);
    return send_json_response(connection, MHD_HTTP_OK, "{\"deleted\":true}");
}

enum MHD_Result handle_request(void *cls, struct MHD_Connection *connection,
                               const char *url, const char *method,
                               const char *version, const char *upload_data,
                               size_t *upload_data_size, void **con_cls) {
    (void)cls;
    (void)version;

    if (!*con_cls) {
        struct connection_info_struct *info = calloc(1, sizeof(*info));
        if (!info) return MHD_NO;
        *con_cls = info;
        return MHD_YES;
    }

    struct connection_info_struct *info = *con_cls;
    char client_ip[INET6_ADDRSTRLEN];
    get_client_ip(connection, client_ip, (socklen_t)sizeof(client_ip));

    if (strcasecmp(method, "POST") == 0 || strcasecmp(method, "PUT") == 0) {
        if (*upload_data_size != 0) {
            if (info->body_too_large || *upload_data_size > MAX_REQUEST_BODY - info->data_len) {
                info->body_too_large = 1;
                *upload_data_size = 0;
                return MHD_YES;
            }

            char *new_data = realloc(info->data, info->data_len + *upload_data_size + 1U);
            if (!new_data) return MHD_NO;
            info->data = new_data;
            memcpy(info->data + info->data_len, upload_data, *upload_data_size);
            info->data_len += *upload_data_size;
            info->data[info->data_len] = '\0';
            *upload_data_size = 0;
            return MHD_YES;
        }

        if (info->body_too_large) {
            return send_error(connection, MHD_HTTP_CONTENT_TOO_LARGE, "Payload too large");
        }
        if (!is_json_content_type(connection)) {
            return send_error(connection, MHD_HTTP_UNSUPPORTED_MEDIA_TYPE, "Content-Type must be application/json");
        }

        cJSON *json = parse_json_body(info);
        if (!json) return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid JSON object");

        int ret;
        if (strcasecmp(method, "POST") == 0 && strcmp(url, "/shorten") == 0) {
            ret = handle_shorten(connection, client_ip, json);
        } else if (strcasecmp(method, "POST") == 0 && strcmp(url, "/api/bulk/shorten") == 0) {
            ret = handle_bulk_shorten(connection, client_ip, json);
        } else if (strcasecmp(method, "POST") == 0 && strncmp(url, "/unlock/", 8) == 0) {
            ret = handle_unlock(connection, client_ip, url, json);
        } else if (strcasecmp(method, "POST") == 0 && strcmp(url, "/api/admin/tokens") == 0) {
            ret = handle_admin_create_token(connection, client_ip, json);
        } else if (strcasecmp(method, "POST") == 0 && strcmp(url, "/api/admin/cleanup") == 0) {
            ret = handle_admin_cleanup(connection, client_ip);
        } else if (strcasecmp(method, "POST") == 0 && strcmp(url, "/api/admin/bulk-action") == 0) {
            ret = handle_admin_bulk_action(connection, client_ip, json);
        } else if (strcasecmp(method, "POST") == 0 && strcmp(url, "/api/admin/restore") == 0) {
            ret = handle_admin_restore(connection, client_ip, json);
        } else if (strcasecmp(method, "POST") == 0 && strcmp(url, "/api/admin/maintenance") == 0) {
            ret = handle_admin_maintenance(connection, client_ip);
        } else if (strncmp(url, "/api/admin/links/", 17) == 0) {
            ret = handle_admin_update_link(connection, client_ip, url + 17, json);
        } else {
            ret = send_error(connection, MHD_HTTP_NOT_FOUND, "Endpoint not found");
        }
        cJSON_Delete(json);
        return ret;
    }

    if (strcasecmp(method, "GET") == 0 || strcasecmp(method, "HEAD") == 0) {
        return handle_get(connection, url, client_ip);
    }

    if (strcasecmp(method, "DELETE") == 0) {
        return handle_delete(connection, url, client_ip);
    }

    if (strcasecmp(method, "OPTIONS") == 0) {
        return send_empty_response(connection, MHD_HTTP_NO_CONTENT);
    }

    return send_error(connection, MHD_HTTP_METHOD_NOT_ALLOWED, "Method not allowed");
}

void request_completed(void *cls, struct MHD_Connection *connection,
                       void **con_cls, enum MHD_RequestTerminationCode toe) {
    (void)cls;
    (void)connection;
    (void)toe;

    struct connection_info_struct *info = *con_cls;
    if (!info) return;
    if (info->data) {
        OPENSSL_cleanse(info->data, info->data_len);
        free(info->data);
    }
    free(info);
    *con_cls = NULL;
}
