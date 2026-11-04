#include "db.h"
#include "handlers.h"
#include "utils.h"
#include <arpa/inet.h>
#include <cjson/cJSON.h>
#include <microhttpd.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

extern const char *g_api_key;
extern const char *g_base_url;

static int failures = 0;

#define CHECK(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "check failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            failures++; \
        } \
    } while (0)

static void cleanup_test_files(const char *directory) {
    char path[512];
    snprintf(path, sizeof(path), "%s/data/shortener.db", directory);
    unlink(path);
    snprintf(path, sizeof(path), "%s/data/shortener.db-wal", directory);
    unlink(path);
    snprintf(path, sizeof(path), "%s/data/shortener.db-shm", directory);
    unlink(path);
    snprintf(path, sizeof(path), "%s/logs/access.log", directory);
    unlink(path);
    snprintf(path, sizeof(path), "%s/logs/error.log", directory);
    unlink(path);
    snprintf(path, sizeof(path), "%s/data", directory);
    rmdir(path);
    snprintf(path, sizeof(path), "%s/logs", directory);
    rmdir(path);
    rmdir(directory);
}

static int send_http_request(uint16_t port, const char *method, const char *path,
                             const char *api_key, const char *json_body,
                             char *response_out, size_t response_out_len) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) != 1 ||
        connect(sock, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close(sock);
        return -1;
    }

    char req[4096];
    size_t body_len = json_body ? strlen(json_body) : 0;
    int req_len = snprintf(
        req, sizeof(req),
        "%s %s HTTP/1.1\r\n"
        "Host: c.micutu.com\r\n"
        "Connection: close\r\n"
        "User-Agent: curl/8.5.0\r\n"
        "%s%s%s"
        "%s"
        "Content-Length: %zu\r\n\r\n"
        "%s",
        method, path,
        api_key ? "X-API-Key: " : "",
        api_key ? api_key : "",
        api_key ? "\r\n" : "",
        json_body ? "Content-Type: application/json\r\n" : "",
        body_len,
        json_body ? json_body : "");
    if (req_len < 0 || (size_t)req_len >= sizeof(req)) {
        close(sock);
        return -1;
    }

    if (send(sock, req, (size_t)req_len, 0) != (ssize_t)req_len) {
        close(sock);
        return -1;
    }

    size_t total_read = 0;
    while (total_read + 1U < response_out_len) {
        ssize_t n = recv(sock, response_out + total_read, response_out_len - 1U - total_read, 0);
        if (n <= 0) break;
        total_read += (size_t)n;
    }
    response_out[total_read] = '\0';
    close(sock);

    int status_code = 0;
    if (sscanf(response_out, "HTTP/%*d.%*d %d", &status_code) != 1) {
        return -1;
    }
    return status_code;
}

static const char *find_http_body(const char *raw_response) {
    const char *sep = strstr(raw_response, "\r\n\r\n");
    return sep ? (sep + 4) : "";
}

int main(void) {
    char directory[64] = "/tmp/shortener-http-test.XXXXXX";
    char old_directory[512];
    char data_path[96];
    char logs_path[96];
    CHECK(getcwd(old_directory, sizeof(old_directory)) != NULL);
    int tmp_fd = mkstemp(directory);
    CHECK(tmp_fd >= 0);
    if (tmp_fd >= 0) {
        close(tmp_fd);
        unlink(directory);
    }
    CHECK(mkdir(directory, 0700) == 0);
    snprintf(data_path, sizeof(data_path), "%s/data", directory);
    snprintf(logs_path, sizeof(logs_path), "%s/logs", directory);
    CHECK(mkdir(data_path, 0700) == 0);
    CHECK(mkdir(logs_path, 0700) == 0);
    CHECK(chdir(directory) == 0);

    const char *admin_key = "test-admin-master-key-0123456789abcdef";
    g_api_key = admin_key;
    g_base_url = "https://c.micutu.com";
    CHECK(init_db("data/shortener.db", "test-hmac-analytics-secret") == 0);

    uint16_t port = 18981;
    struct sockaddr_in listen_addr;
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &listen_addr.sin_addr);

    struct MHD_Daemon *daemon = MHD_start_daemon(
        MHD_USE_AUTO_INTERNAL_THREAD | MHD_USE_ERROR_LOG, 0, NULL, NULL,
        &handle_request, NULL,
        MHD_OPTION_NOTIFY_COMPLETED, request_completed, NULL,
        MHD_OPTION_SOCK_ADDR_LEN, (socklen_t)sizeof(listen_addr),
        (const struct sockaddr *)&listen_addr,
        MHD_OPTION_END);
    CHECK(daemon != NULL);

    if (daemon) {
        char resp[16384];

        // 1. Health & strict CSP headers (no 'unsafe-inline')
        CHECK(send_http_request(port, "GET", "/health", NULL, NULL, resp, sizeof(resp)) == 200);
        CHECK(strstr(resp, "\"status\":\"ok\"") != NULL);
        CHECK(strstr(resp, "script-src 'self'; style-src 'self'") != NULL);
        CHECK(strstr(resp, "unsafe-inline") == NULL);
        CHECK(send_http_request(port, "HEAD", "/health", NULL, NULL, resp, sizeof(resp)) == 200);

        // 2. Static assets & SEO routes
        CHECK(send_http_request(port, "GET", "/assets/styles.css", NULL, NULL, resp, sizeof(resp)) == 200);
        CHECK(send_http_request(port, "GET", "/assets/index.js", NULL, NULL, resp, sizeof(resp)) == 200);
        CHECK(send_http_request(port, "GET", "/favicon.svg", NULL, NULL, resp, sizeof(resp)) == 200);
        CHECK(send_http_request(port, "GET", "/sitemap.xml", NULL, NULL, resp, sizeof(resp)) == 200);
        CHECK(strstr(resp, "<loc>https://c.micutu.com/</loc>") != NULL);
        CHECK(send_http_request(port, "GET", "/robots.txt", NULL, NULL, resp, sizeof(resp)) == 200);
        CHECK(strstr(resp, "Sitemap: https://c.micutu.com/sitemap.xml") != NULL);

        // 3. Unauthorized & SSRF rejection
        CHECK(send_http_request(port, "POST", "/shorten", NULL,
                                "{\"url\":\"https://example.test/ok\"}", resp, sizeof(resp)) == 401);
        CHECK(send_http_request(port, "POST", "/shorten", admin_key,
                                "{\"url\":\"http://127.0.0.1:8086/admin\"}", resp, sizeof(resp)) == 400);
        CHECK(send_http_request(port, "POST", "/shorten", admin_key,
                                "{\"url\":\"https://c.micutu.com/self-loop\"}", resp, sizeof(resp)) == 400);

        // 4. Create Scoped Creator Token and test least-privilege isolation
        CHECK(send_http_request(port, "POST", "/api/admin/tokens", admin_key,
                                "{\"name\":\"CI Bot\",\"ttl_days\":7}", resp, sizeof(resp)) == 201);
        cJSON *tok_json = cJSON_Parse(find_http_body(resp));
        CHECK(tok_json != NULL);
        char creator_token[64] = {0};
        if (tok_json) {
            const char *t = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(tok_json, "token"));
            if (t) snprintf(creator_token, sizeof(creator_token), "%s", t);
            cJSON_Delete(tok_json);
        }
        CHECK(strncmp(creator_token, "csh_", 4U) == 0);

        // Creator token CANNOT access admin endpoints
        CHECK(send_http_request(port, "GET", "/api/admin/links", creator_token, NULL, resp, sizeof(resp)) == 401);

        // Creator token CAN create short links
        CHECK(send_http_request(
                  port, "POST", "/shorten", creator_token,
                  "{\"url\":\"https://example.test/landing\",\"custom_slug\":\"e2e-slug\",\"max_clicks\":1,\"title\":\"E2E\",\"tags\":\"ci,e2e\"}",
                  resp, sizeof(resp)) == 201);

        // 5. QR SVG (including themed QR), Link Inspector, Redirect & 1-Click Burn limit
        CHECK(send_http_request(port, "GET", "/qr/e2e-slug.svg", NULL, NULL, resp, sizeof(resp)) == 200);
        CHECK(strstr(resp, "<svg xmlns=\"http://www.w3.org/2000/svg\"") != NULL);
        CHECK(send_http_request(port, "GET", "/qr/e2e-slug.svg?theme=cyber", NULL, NULL, resp, sizeof(resp)) == 200);
        CHECK(strstr(resp, "fill=\"#38bdf8\"") != NULL);

        CHECK(send_http_request(port, "GET", "/api/inspect/e2e-slug", NULL, NULL, resp, sizeof(resp)) == 200);
        CHECK(strstr(resp, "https://example.test/landing") != NULL);
        CHECK(send_http_request(port, "GET", "/e2e-slug+", NULL, NULL, resp, sizeof(resp)) == 200);

        CHECK(send_http_request(port, "GET", "/e2e-slug", NULL, NULL, resp, sizeof(resp)) == 302);
        CHECK(strstr(resp, "Location: https://example.test/landing") != NULL);

        // Second click on max_clicks=1 link must return 410 Gone
        CHECK(send_http_request(port, "GET", "/e2e-slug", NULL, NULL, resp, sizeof(resp)) == 410);

        // 6. Admin edit link to increase max_clicks and re-test redirect
        CHECK(send_http_request(
                  port, "PUT", "/api/admin/links/e2e-slug", admin_key,
                  "{\"url\":\"https://example.test/landing-v2\",\"max_clicks\":5,\"ttl_hours\":-1,\"is_active\":true,\"tags\":\"ci,e2e\"}",
                  resp, sizeof(resp)) == 200);
        CHECK(send_http_request(port, "GET", "/e2e-slug", NULL, NULL, resp, sizeof(resp)) == 302);
        CHECK(strstr(resp, "Location: https://example.test/landing-v2") != NULL);

        // 7. Stats, CSV Export, Audit Log, and Delete
        CHECK(send_http_request(port, "GET", "/api/stats/e2e-slug", NULL, NULL, resp, sizeof(resp)) == 200);
        CHECK(strstr(resp, "\"total_visits\":2") != NULL);
        CHECK(send_http_request(port, "GET", "/api/admin/export.csv", admin_key, NULL, resp, sizeof(resp)) == 200);
        CHECK(strstr(resp, "\"e2e-slug\"") != NULL);
        CHECK(send_http_request(port, "GET", "/api/admin/audit", admin_key, NULL, resp, sizeof(resp)) == 200);
        CHECK(strstr(resp, "link.create") != NULL);

        // 8. Social Preview Card (/p/<slug>), Link-in-Bio Hub (/bio/<tag>), Bulk Shorten, Metrics & System Diagnostics
        CHECK(send_http_request(port, "GET", "/p/e2e-slug", NULL, NULL, resp, sizeof(resp)) == 200);
        CHECK(strstr(resp, "property=\"og:title\"") != NULL);
        CHECK(send_http_request(port, "GET", "/bio/e2e", NULL, NULL, resp, sizeof(resp)) == 200);
        CHECK(strstr(resp, "/e2e-slug") != NULL);
        CHECK(send_http_request(port, "GET", "/api/bio/e2e", NULL, NULL, resp, sizeof(resp)) == 200);
        CHECK(strstr(resp, "\"profile\":\"e2e\"") != NULL);

        CHECK(send_http_request(
                  port, "POST", "/api/bulk/shorten", creator_token,
                  "{\"links\":[{\"url\":\"https://example.test/b1\",\"custom_slug\":\"bulk-1\",\"tags\":\"batch\"},{\"url\":\"https://example.test/b2\",\"custom_slug\":\"bulk-2\",\"tags\":\"batch\"}]}",
                  resp, sizeof(resp)) == 201);
        CHECK(strstr(resp, "\"created\":2") != NULL);

        CHECK(send_http_request(port, "GET", "/metrics", NULL, NULL, resp, sizeof(resp)) == 401);
        CHECK(send_http_request(port, "GET", "/metrics", admin_key, NULL, resp, sizeof(resp)) == 200);
        CHECK(strstr(resp, "shortener_links_total") != NULL);
        CHECK(send_http_request(port, "GET", "/api/admin/system", admin_key, NULL, resp, sizeof(resp)) == 200);
        CHECK(strstr(resp, "\"sqlite_version\"") != NULL);
        CHECK(send_http_request(port, "GET", "/api/admin/backup.json", admin_key, NULL, resp, sizeof(resp)) == 200);
        CHECK(strstr(resp, "\"c.micutu.com-snapshot-v1\"") != NULL);
        CHECK(send_http_request(
                  port, "POST", "/api/admin/bulk-action", admin_key,
                  "{\"action\":\"disable\",\"slugs\":[\"bulk-1\",\"bulk-2\"]}",
                  resp, sizeof(resp)) == 200);
        CHECK(strstr(resp, "\"affected\":2") != NULL);
        CHECK(send_http_request(port, "GET", "/bulk-1", NULL, NULL, resp, sizeof(resp)) == 410);

        CHECK(send_http_request(port, "DELETE", "/e2e-slug", admin_key, NULL, resp, sizeof(resp)) == 200);
        CHECK(send_http_request(port, "GET", "/e2e-slug", NULL, NULL, resp, sizeof(resp)) == 404);

        MHD_stop_daemon(daemon);
    }

    close_db();
    CHECK(chdir(old_directory) == 0);
    cleanup_test_files(directory);

    if (failures != 0) return 1;
    puts("test_http: ok");
    return 0;
}
