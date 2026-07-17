#include "handlers.h"
#include "db.h"
#include "utils.h"
#include "index_html.h"
#include "password_html.h"
#include "stats_html.h"
#include "error_html.h"
#include "admin_html.h"
#include <arpa/inet.h>
#include <cjson/cJSON.h>
#include <ctype.h>
#include <math.h>
#include <openssl/crypto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define MAX_URL_LEN 2048U
#define MAX_URL_BUFFER (MAX_URL_LEN + 1U)
#define MAX_SLUG_LEN 32U
#define MAX_SLUG_LENGTH (MAX_SLUG_LEN - 1U)
#define MAX_PASSWORD_LEN 128U
#define MAX_REQUEST_BODY 8192U
#define MAX_USER_AGENT_LEN 512U
#define MAX_TTL_HOURS 8760
#define MAX_FULL_URL_LEN 512U

#define CREATE_RATE_LIMIT 20
#define UNLOCK_RATE_LIMIT 5
#define STATS_RATE_LIMIT 60
#define ADMIN_RATE_LIMIT 30

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
    MHD_add_response_header(response, "Referrer-Policy", "no-referrer");
    MHD_add_response_header(response, "Permissions-Policy", "geolocation=(), microphone=(), camera=()");
    MHD_add_response_header(response, "Cross-Origin-Opener-Policy", "same-origin");
    MHD_add_response_header(response, "Cross-Origin-Resource-Policy", "same-origin");
    MHD_add_response_header(response, "Cache-Control", "no-store, max-age=0");
    MHD_add_response_header(response, "Pragma", "no-cache");
    MHD_add_response_header(response, "Content-Security-Policy",
                            "default-src 'self'; base-uri 'none'; form-action 'self'; "
                            "frame-ancestors 'none'; object-src 'none'; connect-src 'self'; "
                            "img-src 'self' data:; script-src 'self' 'unsafe-inline'; "
                            "style-src 'self' 'unsafe-inline'");
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

    int html_len = snprintf(NULL, 0, ERROR_HTML_TEMPLATE, page_title, code, title, desc);
    if (html_len < 0) return send_error(connection, code, "Request failed");

    char *html = malloc((size_t)html_len + 1U);
    if (!html) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error");
    snprintf(html, (size_t)html_len + 1U, ERROR_HTML_TEMPLATE, page_title, code, title, desc);
    return send_html_dynamic(connection, code, html);
}

static int api_key_is_configured(void) {
    return g_api_key && g_api_key[0] != '\0';
}

static int check_api_key(struct MHD_Connection *connection) {
    if (!api_key_is_configured()) return 0;

    const char *provided = MHD_lookup_connection_value(
        connection, MHD_HEADER_KIND, "X-API-Key");
    if (!provided) return 0;

    size_t expected_len = strlen(g_api_key);
    size_t provided_len = strlen(provided);
    if (expected_len != provided_len) return 0;
    return CRYPTO_memcmp(g_api_key, provided, expected_len) == 0;
}

static int is_reserved_slug(const char *slug) {
    static const char *const reserved[] = {
        "admin", "api", "favicon.ico", "health", "robots.txt", "stats", "unlock"
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

static int get_optional_password(cJSON *json, const char **password_out) {
    cJSON *password = cJSON_GetObjectItemCaseSensitive(json, "password");
    *password_out = NULL;
    if (!password || cJSON_IsNull(password)) return 0;
    if (!cJSON_IsString(password) || !password->valuestring) return -1;

    size_t password_len = strnlen(password->valuestring, MAX_PASSWORD_LEN + 1U);
    if (password_len > MAX_PASSWORD_LEN) return -1;
    if (password_len > 0) *password_out = password->valuestring;
    return 0;
}

static int get_ttl_hours(cJSON *json, int *ttl_out) {
    cJSON *ttl = cJSON_GetObjectItemCaseSensitive(json, "ttl_hours");
    *ttl_out = 0;
    if (!ttl || cJSON_IsNull(ttl)) return 0;
    if (!cJSON_IsNumber(ttl) || !isfinite(ttl->valuedouble) ||
        floor(ttl->valuedouble) != ttl->valuedouble || ttl->valuedouble < 0 ||
        ttl->valuedouble > MAX_TTL_HOURS) {
        return -1;
    }

    *ttl_out = (int)ttl->valuedouble;
    return 0;
}

static int handle_shorten(struct MHD_Connection *connection, const char *client_ip,
                          cJSON *json) {
    if (!check_api_key(connection)) {
        return send_error(connection, MHD_HTTP_UNAUTHORIZED, "Invalid or missing API key");
    }
    if (check_rate_limit(client_ip, CREATE_RATE_LIMIT) != 0) {
        return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
    }

    cJSON *target_url = cJSON_GetObjectItemCaseSensitive(json, "url");
    cJSON *custom_slug = cJSON_GetObjectItemCaseSensitive(json, "custom_slug");
    if (!cJSON_IsString(target_url) || !target_url->valuestring ||
        !is_valid_http_url(target_url->valuestring, MAX_URL_LEN)) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "A valid HTTP or HTTPS URL is required");
    }

    int ttl_hours;
    if (get_ttl_hours(json, &ttl_hours) != 0) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "ttl_hours must be a whole number from 0 to 8760");
    }

    const char *password = NULL;
    if (get_optional_password(json, &password) != 0) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "Password is too long or invalid");
    }

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
        if (!custom_slug_requested) generate_random_slug(slug, 8);
        insert_result = insert_link(slug, target_url->valuestring, ttl_hours, password);
        if (insert_result == 0) break;
    }
    if (insert_result != 0) {
        return send_error(connection, MHD_HTTP_CONFLICT,
                          custom_slug_requested ? "Custom slug already exists" : "Could not allocate a unique slug");
    }

    char full_url[MAX_FULL_URL_LEN];
    int full_url_len = snprintf(full_url, sizeof(full_url), "%s/%s", g_base_url, slug);
    if (full_url_len < 0 || (size_t)full_url_len >= sizeof(full_url)) {
        log_error("Configured base URL is too long");
        return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Service configuration error");
    }

    cJSON *response_json = cJSON_CreateObject();
    if (!response_json || !cJSON_AddStringToObject(response_json, "short_url", full_url)) {
        cJSON_Delete(response_json);
        return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error");
    }
    char *response_text = cJSON_PrintUnformatted(response_json);
    cJSON_Delete(response_json);
    if (!response_text) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error");

    int ret = send_json_response(connection, MHD_HTTP_CREATED, response_text);
    free(response_text);
    log_message("Created short link: %s", slug);
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
                                           target_url, (int)sizeof(target_url));
    if (db_result == -2) return send_error(connection, MHD_HTTP_GONE, "Link expired");
    if (db_result == -4) return send_error(connection, MHD_HTTP_UNAUTHORIZED, "Wrong password");
    if (db_result != 0 || !is_valid_http_url(target_url, MAX_URL_LEN)) {
        return send_error(connection, MHD_HTTP_NOT_FOUND, "Not found");
    }

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

static int copy_user_agent(struct MHD_Connection *connection, char *output, size_t output_len) {
    const char *user_agent = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "User-Agent");
    if (!user_agent) user_agent = "Unknown";

    size_t len = strnlen(user_agent, output_len);
    if (len >= output_len) len = output_len - 1U;
    memcpy(output, user_agent, len);
    output[len] = '\0';
    return 0;
}

static int handle_get(struct MHD_Connection *connection, const char *url, const char *client_ip) {
    if (strcmp(url, "/health") == 0) {
        return send_json_response(connection, MHD_HTTP_OK, "{\"status\":\"ok\"}");
    }

    if (strcmp(url, "/robots.txt") == 0) {
        static const char robots[] = "User-agent: *\nDisallow: /stats/\nDisallow: /admin\nDisallow: /api/\n";
        return send_text_response(connection, MHD_HTTP_OK, robots, "text/plain; charset=utf-8");
    }

    if (strcmp(url, "/favicon.ico") == 0) {
        return send_empty_response(connection, MHD_HTTP_NO_CONTENT);
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
        if (!check_api_key(connection)) {
            return send_error(connection, MHD_HTTP_UNAUTHORIZED, "Invalid or missing API key");
        }
        if (check_rate_limit(client_ip, ADMIN_RATE_LIMIT) != 0) {
            return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
        }

        char *json = get_all_links_json();
        if (!json) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Database error");
        int ret = send_json_response(connection, MHD_HTTP_OK, json);
        free(json);
        return ret;
    }

    if (strncmp(url, "/stats/", 7) == 0) {
        const char *slug = url + 7;
        if (!is_valid_slug(slug, MAX_SLUG_LENGTH)) {
            return send_error_page(connection, MHD_HTTP_BAD_REQUEST, "Bad Request", "The link identifier is invalid.");
        }
        if (check_rate_limit(client_ip, STATS_RATE_LIMIT) != 0) {
            return send_error_page(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Too Many Requests", "Please wait a moment and try again.");
        }

        int requires_password = 0;
        char ignored_url[MAX_URL_BUFFER];
        int db_result = get_link(slug, ignored_url, (int)sizeof(ignored_url), &requires_password);
        if (db_result == -1) return send_error_page(connection, MHD_HTTP_NOT_FOUND, "Not Found", "This short link does not exist.");
        if (db_result == -2) return send_error_page(connection, MHD_HTTP_GONE, "Link Expired", "This short link has expired.");

        int total = 0;
        int unique = 0;
        if (get_stats(slug, &total, &unique) != 0) {
            return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Database error");
        }

        int html_len = snprintf(NULL, 0, STATS_HTML, slug, total, unique);
        if (html_len < 0) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Rendering error");
        char *html = malloc((size_t)html_len + 1U);
        if (!html) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error");
        snprintf(html, (size_t)html_len + 1U, STATS_HTML, slug, total, unique);
        return send_html_dynamic(connection, MHD_HTTP_OK, html);
    }

    if (strcmp(url, "/") == 0) {
        return send_html(connection, MHD_HTTP_OK, INDEX_HTML);
    }

    const char *slug = url + 1;
    if (!is_valid_slug(slug, MAX_SLUG_LENGTH)) {
        return send_error_page(connection, MHD_HTTP_NOT_FOUND, "Not Found", "This short link does not exist.");
    }

    char target_url[MAX_URL_BUFFER];
    int requires_password = 0;
    int db_result = get_link(slug, target_url, (int)sizeof(target_url), &requires_password);
    if (db_result == -2) {
        return send_error_page(connection, MHD_HTTP_GONE, "Link Expired", "This short link has expired.");
    }
    if (db_result != 0) {
        return send_error_page(connection, MHD_HTTP_NOT_FOUND, "Not Found", "This short link does not exist.");
    }

    if (requires_password) return send_html(connection, MHD_HTTP_OK, PASSWORD_HTML);
    if (!is_valid_http_url(target_url, MAX_URL_LEN)) {
        log_error("Refused redirect for malformed stored URL on slug %s", slug);
        return send_error_page(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Unavailable", "This link cannot be redirected safely.");
    }

    char user_agent[MAX_USER_AGENT_LEN + 1U];
    copy_user_agent(connection, user_agent, sizeof(user_agent));
    if (record_visit(slug, client_ip, user_agent) != 0) {
        log_error("Could not record visit for %s", slug);
    }
    log_message("Redirected short link: %s", slug);

    static const char empty[] = "";
    struct MHD_Response *response = MHD_create_response_from_buffer(
        0, (void *)empty, MHD_RESPMEM_PERSISTENT);
    if (!response) return MHD_NO;
    MHD_add_response_header(response, "Location", target_url);
    add_security_headers(response);
    int ret = MHD_queue_response(connection, MHD_HTTP_FOUND, response);
    MHD_destroy_response(response);
    return ret;
}

static int handle_delete(struct MHD_Connection *connection, const char *url,
                         const char *client_ip) {
    if (!check_api_key(connection)) {
        return send_error(connection, MHD_HTTP_UNAUTHORIZED, "Invalid or missing API key");
    }
    if (check_rate_limit(client_ip, ADMIN_RATE_LIMIT) != 0) {
        return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
    }

    const char *slug = url + 1;
    if (!is_valid_slug(slug, MAX_SLUG_LENGTH) || is_reserved_slug(slug)) {
        return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid slug");
    }

    if (delete_link(slug) != 0) return send_error(connection, MHD_HTTP_NOT_FOUND, "Slug not found");

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

    if (strcasecmp(method, "POST") == 0) {
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
        if (strcmp(url, "/shorten") == 0) {
            ret = handle_shorten(connection, client_ip, json);
        } else if (strncmp(url, "/unlock/", 8) == 0) {
            ret = handle_unlock(connection, client_ip, url, json);
        } else {
            ret = send_error(connection, MHD_HTTP_NOT_FOUND, "Endpoint not found");
        }
        cJSON_Delete(json);
        return ret;
    }

    if (strcasecmp(method, "GET") == 0) {
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
    free(info->data);
    free(info);
    *con_cls = NULL;
}
