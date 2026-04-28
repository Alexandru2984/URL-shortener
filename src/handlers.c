#include "handlers.h"
#include "db.h"
#include "utils.h"
#include "index_html.h"
#include <cjson/cJSON.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_URL_LEN 2048
#define MAX_SLUG_LEN 32

struct connection_info_struct {
    int connection_type;
    char *data;
    size_t data_len;
};

#define GET 0
#define POST 1

static int send_json_response(struct MHD_Connection *connection, int status_code, const char *json_str) {
    struct MHD_Response *response;
    int ret;
    response = MHD_create_response_from_buffer(strlen(json_str), (void *)json_str, MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header(response, "Content-Type", "application/json");
    ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    return ret;
}

static int send_error(struct MHD_Connection *connection, int status_code, const char *message) {
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "error", message);
    char *json_str = cJSON_PrintUnformatted(json);
    int ret = send_json_response(connection, status_code, json_str);
    free(json_str);
    cJSON_Delete(json);
    return ret;
}

enum MHD_Result handle_request(void *cls, struct MHD_Connection *connection,
                               const char *url, const char *method,
                               const char *version, const char *upload_data,
                               size_t *upload_data_size, void **con_cls) {
    (void)cls;
    (void)version;

    if (NULL == *con_cls) {
        struct connection_info_struct *con_info;
        con_info = malloc(sizeof(struct connection_info_struct));
        if (NULL == con_info) return MHD_NO;
        con_info->data = NULL;
        con_info->data_len = 0;

        if (strcasecmp(method, "POST") == 0) {
            con_info->connection_type = POST;
        } else {
            con_info->connection_type = GET;
        }
        *con_cls = (void *)con_info;
        return MHD_YES;
    }

    struct connection_info_struct *con_info = *con_cls;

    if (strcasecmp(method, "POST") == 0) {
        if (*upload_data_size != 0) {
            if (con_info->data_len + *upload_data_size > 10240) { // Limit to 10KB
                return send_error(connection, MHD_HTTP_PAYLOAD_TOO_LARGE, "Payload too large");
            }
            char *new_data = realloc(con_info->data, con_info->data_len + *upload_data_size + 1);
            if (!new_data) return MHD_NO;
            con_info->data = new_data;
            memcpy(con_info->data + con_info->data_len, upload_data, *upload_data_size);
            con_info->data_len += *upload_data_size;
            con_info->data[con_info->data_len] = '\0';
            *upload_data_size = 0;
            return MHD_YES;
        } else {
            // POST request finished, process data
            if (!con_info->data) {
                return send_error(connection, MHD_HTTP_BAD_REQUEST, "Empty body");
            }

            // Extract IP for rate limiting
            const char *ip_addr = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "X-Real-IP");
            if (!ip_addr) ip_addr = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "X-Forwarded-For");
            if (!ip_addr) {
                // simplified fallback
                ip_addr = "127.0.0.1";
            }
            
            if (check_rate_limit(ip_addr, 60) != 0) { // 60 requests per minute
                return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
            }

            cJSON *json = cJSON_Parse(con_info->data);
            if (!json) {
                return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid JSON");
            }

            cJSON *target_url = cJSON_GetObjectItemCaseSensitive(json, "url");
            cJSON *custom_slug = cJSON_GetObjectItemCaseSensitive(json, "custom_slug");

            if (!cJSON_IsString(target_url) || (target_url->valuestring == NULL)) {
                cJSON_Delete(json);
                return send_error(connection, MHD_HTTP_BAD_REQUEST, "Missing or invalid 'url' field");
            }

            // Basic validation
            if (strncmp(target_url->valuestring, "http://", 7) != 0 && strncmp(target_url->valuestring, "https://", 8) != 0) {
                cJSON_Delete(json);
                return send_error(connection, MHD_HTTP_BAD_REQUEST, "URL must start with http:// or https://");
            }

            char slug[MAX_SLUG_LEN];
            if (cJSON_IsString(custom_slug) && custom_slug->valuestring != NULL && strlen(custom_slug->valuestring) > 0) {
                strncpy(slug, custom_slug->valuestring, MAX_SLUG_LEN - 1);
                slug[MAX_SLUG_LEN - 1] = '\0';
            } else {
                generate_random_slug(slug, 6);
            }

            if (insert_link(slug, target_url->valuestring) != 0) {
                cJSON_Delete(json);
                return send_error(connection, MHD_HTTP_CONFLICT, "Slug already exists or DB error");
            }

            cJSON *resp_json = cJSON_CreateObject();
            char full_url[256];
            snprintf(full_url, sizeof(full_url), "https://c.micutu.com/%s", slug);
            cJSON_AddStringToObject(resp_json, "short_url", full_url);
            char *resp_str = cJSON_PrintUnformatted(resp_json);

            int ret = send_json_response(connection, MHD_HTTP_CREATED, resp_str);
            
            log_message("Shortened %s to %s", target_url->valuestring, slug);

            free(resp_str);
            cJSON_Delete(resp_json);
            cJSON_Delete(json);

            return ret;
        }
    }

    if (strcasecmp(method, "GET") == 0) {
        // Extract IP and UA
        const char *ip_addr = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "X-Real-IP");
        if (!ip_addr) ip_addr = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "X-Forwarded-For");
        if (!ip_addr) ip_addr = "127.0.0.1"; // Default fallback
        const char *user_agent = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "User-Agent");
        if (!user_agent) user_agent = "Unknown";

        if (strncmp(url, "/stats/", 7) == 0) {
            const char *slug = url + 7;
            if (strlen(slug) == 0) return send_error(connection, MHD_HTTP_BAD_REQUEST, "Missing slug");

            if (check_rate_limit(ip_addr, 120) != 0) {
                return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
            }

            char dummy[MAX_URL_LEN];
            if (get_link(slug, dummy, sizeof(dummy)) != 0) {
                return send_error(connection, MHD_HTTP_NOT_FOUND, "Slug not found");
            }

            int total, unique;
            get_stats(slug, &total, &unique);

            cJSON *resp_json = cJSON_CreateObject();
            cJSON_AddNumberToObject(resp_json, "total_visits", total);
            cJSON_AddNumberToObject(resp_json, "unique_visitors", unique);
            char *resp_str = cJSON_PrintUnformatted(resp_json);
            int ret = send_json_response(connection, MHD_HTTP_OK, resp_str);
            free(resp_str);
            cJSON_Delete(resp_json);
            return ret;
        }

        // Handle redirect
        const char *slug = url + 1; // Skip leading '/'
        if (strlen(slug) == 0) {
             const char *html = INDEX_HTML;
             struct MHD_Response *response = MHD_create_response_from_buffer(strlen(html), (void *)html, MHD_RESPMEM_PERSISTENT);
             MHD_add_response_header(response, "Content-Type", "text/html; charset=utf-8");
             int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
             MHD_destroy_response(response);
             return ret;
        }

        char target_url[MAX_URL_LEN];
        if (get_link(slug, target_url, sizeof(target_url)) == 0) {
            record_visit(slug, ip_addr, user_agent);
            log_message("Redirected %s to %s", slug, target_url);

            struct MHD_Response *response = MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Location", target_url);
            int ret = MHD_queue_response(connection, MHD_HTTP_FOUND, response);
            MHD_destroy_response(response);
            return ret;
        } else {
            return send_error(connection, MHD_HTTP_NOT_FOUND, "Slug not found");
        }
    }

    return send_error(connection, MHD_HTTP_METHOD_NOT_ALLOWED, "Method not allowed");
}

void request_completed(void *cls, struct MHD_Connection *connection,
                       void **con_cls, enum MHD_RequestTerminationCode toe) {
    (void)cls;
    (void)connection;
    (void)toe;
    struct connection_info_struct *con_info = *con_cls;
    if (NULL == con_info) return;
    if (con_info->data) free(con_info->data);
    free(con_info);
    *con_cls = NULL;
}
