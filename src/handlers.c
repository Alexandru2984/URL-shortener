#include "handlers.h"
#include "db.h"
#include "utils.h"
#include "index_html.h"
#include "password_html.h"
#include "stats_html.h"
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

static int send_html(struct MHD_Connection *connection, int status_code, const char *html) {
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(html), (void *)html, MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(response, "Content-Type", "text/html; charset=utf-8");
    int ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
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

    // Extract IP for rate limiting
    const char *ip_addr = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "X-Real-IP");
    if (!ip_addr) ip_addr = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "X-Forwarded-For");
    if (!ip_addr) ip_addr = "127.0.0.1";

    if (strcasecmp(method, "POST") == 0) {
        if (*upload_data_size != 0) {
            if (con_info->data_len + *upload_data_size > 10240) { // Limit to 10KB
                return send_error(connection, MHD_HTTP_CONTENT_TOO_LARGE, "Payload too large");
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
            if (!con_info->data) return send_error(connection, MHD_HTTP_BAD_REQUEST, "Empty body");

            if (check_rate_limit(ip_addr, 60) != 0) {
                return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
            }

            cJSON *json = cJSON_Parse(con_info->data);
            if (!json) return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid JSON");

            if (strcmp(url, "/shorten") == 0) {
                cJSON *target_url = cJSON_GetObjectItemCaseSensitive(json, "url");
                cJSON *custom_slug = cJSON_GetObjectItemCaseSensitive(json, "custom_slug");
                cJSON *ttl_hours_json = cJSON_GetObjectItemCaseSensitive(json, "ttl_hours");
                cJSON *password_json = cJSON_GetObjectItemCaseSensitive(json, "password");

                if (!cJSON_IsString(target_url) || (target_url->valuestring == NULL)) {
                    cJSON_Delete(json);
                    return send_error(connection, MHD_HTTP_BAD_REQUEST, "Missing 'url'");
                }

                if (strncmp(target_url->valuestring, "http://", 7) != 0 && strncmp(target_url->valuestring, "https://", 8) != 0) {
                    cJSON_Delete(json);
                    return send_error(connection, MHD_HTTP_BAD_REQUEST, "Invalid schema");
                }

                char base_slug[MAX_SLUG_LEN];
                char slug[MAX_SLUG_LEN];
                int is_custom = 0;
                
                if (cJSON_IsString(custom_slug) && custom_slug->valuestring && strlen(custom_slug->valuestring) > 0) {
                    slugify(custom_slug->valuestring, base_slug, MAX_SLUG_LEN);
                    if (strlen(base_slug) == 0) {
                        generate_random_slug(slug, 6);
                    } else {
                        strcpy(slug, base_slug);
                        is_custom = 1;
                    }
                } else {
                    generate_random_slug(slug, 6);
                }

                int ttl = 0;
                if (cJSON_IsNumber(ttl_hours_json)) ttl = ttl_hours_json->valueint;
                
                const char *pwd = NULL;
                if (cJSON_IsString(password_json) && password_json->valuestring && strlen(password_json->valuestring) > 0) {
                    pwd = password_json->valuestring;
                }

                int insert_res = insert_link(slug, target_url->valuestring, ttl, pwd);
                if (insert_res != 0 && is_custom) {
                    for (int attempts = 0; attempts < 10; attempts++) {
                        char suffix[6];
                        generate_random_slug(suffix, 4);
                        snprintf(slug, MAX_SLUG_LEN, "%.*s-%s", MAX_SLUG_LEN - 6, base_slug, suffix);
                        insert_res = insert_link(slug, target_url->valuestring, ttl, pwd);
                        if (insert_res == 0) break;
                    }
                }

                if (insert_res != 0) {
                    cJSON_Delete(json);
                    return send_error(connection, MHD_HTTP_CONFLICT, "Slug exists or DB error");
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
            } else if (strncmp(url, "/unlock/", 8) == 0) {
                // Password unlock endpoint
                const char *slug = url + 8;
                cJSON *password_json = cJSON_GetObjectItemCaseSensitive(json, "password");
                const char *pwd = cJSON_IsString(password_json) ? password_json->valuestring : "";

                char target_url[MAX_URL_LEN];
                int db_res = get_link_with_password(slug, pwd, target_url, sizeof(target_url));
                cJSON_Delete(json);

                if (db_res == 0) {
                    cJSON *resp_json = cJSON_CreateObject();
                    cJSON_AddStringToObject(resp_json, "url", target_url);
                    char *resp_str = cJSON_PrintUnformatted(resp_json);
                    int ret = send_json_response(connection, MHD_HTTP_OK, resp_str);
                    free(resp_str);
                    cJSON_Delete(resp_json);
                    return ret;
                } else if (db_res == -2) {
                    return send_error(connection, MHD_HTTP_GONE, "Link expired");
                } else if (db_res == -4) {
                    return send_error(connection, MHD_HTTP_UNAUTHORIZED, "Wrong password");
                } else {
                    return send_error(connection, MHD_HTTP_NOT_FOUND, "Not found");
                }
            } else {
                cJSON_Delete(json);
                return send_error(connection, MHD_HTTP_NOT_FOUND, "Endpoint not found");
            }
        }
    }

    if (strcasecmp(method, "GET") == 0) {
        const char *user_agent = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "User-Agent");
        if (!user_agent) user_agent = "Unknown";

        if (strncmp(url, "/stats/", 7) == 0) {
            const char *slug = url + 7;
            if (strlen(slug) == 0) return send_error(connection, MHD_HTTP_BAD_REQUEST, "Missing slug");

            if (check_rate_limit(ip_addr, 120) != 0) {
                return send_error(connection, MHD_HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
            }

            int req_pwd;
            char dummy[MAX_URL_LEN];
            int db_res = get_link(slug, dummy, sizeof(dummy), &req_pwd);
            if (db_res == -1) return send_error(connection, MHD_HTTP_NOT_FOUND, "Slug not found");
            if (db_res == -2) return send_error(connection, MHD_HTTP_GONE, "Link expired");

            int total, unique;
            get_stats(slug, &total, &unique);

            // Calculate length required for HTML response
            int html_len = snprintf(NULL, 0, STATS_HTML, slug, total, unique);
            char *html_buf = malloc(html_len + 1);
            if (!html_buf) return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error");
            snprintf(html_buf, html_len + 1, STATS_HTML, slug, total, unique);

            struct MHD_Response *response = MHD_create_response_from_buffer(html_len, (void *)html_buf, MHD_RESPMEM_MUST_FREE);
            MHD_add_response_header(response, "Content-Type", "text/html; charset=utf-8");
            int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
            return ret;
        }

        // Handle redirect
        const char *slug = url + 1; // Skip leading '/'
        if (strlen(slug) == 0) {
             return send_html(connection, MHD_HTTP_OK, INDEX_HTML);
        }

        char target_url[MAX_URL_LEN];
        int req_pwd = 0;
        int db_res = get_link(slug, target_url, sizeof(target_url), &req_pwd);
        
        if (db_res == -2) {
             return send_error(connection, MHD_HTTP_GONE, "Link has expired");
        } else if (db_res == -1) {
             return send_error(connection, MHD_HTTP_NOT_FOUND, "Slug not found");
        }

        if (req_pwd) {
             // Return password HTML page, replacing the %s with the slug
             // For simplicity, we just return the static HTML, it will figure out the slug from window.location.pathname
             return send_html(connection, MHD_HTTP_OK, PASSWORD_HTML);
        }

        // Proceed to redirect
        record_visit(slug, ip_addr, user_agent);
        log_message("Redirected %s to %s", slug, target_url);

        struct MHD_Response *response = MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Location", target_url);
        int ret = MHD_queue_response(connection, MHD_HTTP_FOUND, response);
        MHD_destroy_response(response);
        return ret;
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
