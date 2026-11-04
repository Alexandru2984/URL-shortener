#include <arpa/inet.h>
#include <cjson/cJSON.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static void print_usage(const char *prog) {
    fprintf(stderr,
            "c.micutu.com Pure-C CLI Client (C17)\n"
            "Usage:\n"
            "  %s shorten <url> [--slug <slug>] [--title <title>] [--tag <tags>]\n"
            "                   [--mobile <mobile_url>] [--alt-url <url> --alt-weight <1-99>]\n"
            "                   [--ttl <hours>] [--max-clicks <n>] [--key <api_key_or_token>]\n"
            "  %s inspect <slug>\n"
            "  %s stats <slug>\n"
            "  %s bio <tag>\n"
            "  %s qr <slug> [output.svg]\n"
            "  %s health\n\n"
            "Environment variables:\n"
            "  SHORTENER_HOST     Target host (default: 127.0.0.1)\n"
            "  SHORTENER_PORT     Target port (default: 8086)\n"
            "  SHORTENER_API_KEY  API Key or Scoped Creator Token (csh_...)\n",
            prog, prog, prog, prog, prog, prog);
}

static int http_request(const char *host, const char *port, const char *method,
                        const char *path, const char *api_key, const char *json_body,
                        char *resp_out, size_t resp_cap, int *status_out) {
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) != 0 || !res) {
        return -1;
    }

    int sock = -1;
    for (struct addrinfo *p = res; p != NULL; p = p->ai_next) {
        sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sock < 0) continue;
        if (connect(sock, p->ai_addr, p->ai_addrlen) == 0) break;
        close(sock);
        sock = -1;
    }
    freeaddrinfo(res);
    if (sock < 0) return -1;

    size_t body_len = json_body ? strlen(json_body) : 0U;
    char req[8192];
    int req_len = snprintf(
        req, sizeof(req),
        "%s %s HTTP/1.1\r\n"
        "Host: %s:%s\r\n"
        "User-Agent: shortener-cli/1.0 (C17)\r\n"
        "Connection: close\r\n"
        "%s%s%s"
        "%s"
        "Content-Length: %zu\r\n\r\n"
        "%s",
        method, path, host, port,
        (api_key && api_key[0]) ? "X-API-Key: " : "",
        (api_key && api_key[0]) ? api_key : "",
        (api_key && api_key[0]) ? "\r\n" : "",
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

    size_t total = 0U;
    while (total + 1U < resp_cap) {
        ssize_t n = recv(sock, resp_out + total, resp_cap - 1U - total, 0);
        if (n <= 0) break;
        total += (size_t)n;
    }
    resp_out[total] = '\0';
    close(sock);

    int code = 0;
    if (sscanf(resp_out, "HTTP/%*d.%*d %d", &code) != 1) return -1;
    if (status_out) *status_out = code;
    return 0;
}

static const char *extract_body(const char *raw) {
    const char *sep = strstr(raw, "\r\n\r\n");
    return sep ? (sep + 4) : raw;
}

int main(int argc, char **argv) {
    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_usage(argv[0]);
        return argc < 2 ? 1 : 0;
    }

    const char *host = getenv("SHORTENER_HOST");
    if (!host || !host[0]) host = "127.0.0.1";
    const char *port = getenv("SHORTENER_PORT");
    if (!port || !port[0]) port = "8086";
    const char *api_key = getenv("SHORTENER_API_KEY");

    const char *cmd = argv[1];
    char resp[32768];
    int status = 0;

    if (strcmp(cmd, "health") == 0) {
        if (http_request(host, port, "GET", "/health", NULL, NULL, resp, sizeof(resp), &status) != 0) {
            fprintf(stderr, "Error: could not connect to %s:%s\n", host, port);
            return 1;
        }
        printf("HTTP %d: %s\n", status, extract_body(resp));
        return status == 200 ? 0 : 1;
    }

    if (strcmp(cmd, "inspect") == 0 || strcmp(cmd, "stats") == 0 || strcmp(cmd, "bio") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: missing identifier\n");
            return 1;
        }
        char path[256];
        snprintf(path, sizeof(path), "/api/%s/%s", cmd, argv[2]);
        if (http_request(host, port, "GET", path, NULL, NULL, resp, sizeof(resp), &status) != 0) {
            fprintf(stderr, "Error: could not connect to %s:%s\n", host, port);
            return 1;
        }
        cJSON *parsed = cJSON_Parse(extract_body(resp));
        if (parsed) {
            char *pretty = cJSON_Print(parsed);
            if (pretty) {
                puts(pretty);
                free(pretty);
            }
            cJSON_Delete(parsed);
        } else {
            puts(extract_body(resp));
        }
        return status == 200 ? 0 : 1;
    }

    if (strcmp(cmd, "qr") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: missing <slug>\n");
            return 1;
        }
        char path[256];
        snprintf(path, sizeof(path), "/qr/%s.svg", argv[2]);
        if (http_request(host, port, "GET", path, NULL, NULL, resp, sizeof(resp), &status) != 0 || status != 200) {
            fprintf(stderr, "Error: failed to fetch QR code (HTTP %d)\n", status);
            return 1;
        }
        const char *svg = extract_body(resp);
        if (argc >= 4) {
            FILE *fp = fopen(argv[3], "wb");
            if (!fp) {
                perror("fopen");
                return 1;
            }
            fputs(svg, fp);
            fclose(fp);
            printf("Saved SVG QR code to %s\n", argv[3]);
        } else {
            puts(svg);
        }
        return 0;
    }

    if (strcmp(cmd, "shorten") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: missing <url>\n");
            return 1;
        }
        const char *target_url = argv[2];
        const char *slug = NULL;
        const char *title = NULL;
        const char *tags = NULL;
        const char *mobile_url = NULL;
        const char *alt_url = NULL;
        int alt_weight = 0;
        int ttl_hours = 0;
        int max_clicks = 0;

        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "--slug") == 0 && i + 1 < argc) slug = argv[++i];
            else if (strcmp(argv[i], "--title") == 0 && i + 1 < argc) title = argv[++i];
            else if (strcmp(argv[i], "--tag") == 0 && i + 1 < argc) tags = argv[++i];
            else if (strcmp(argv[i], "--mobile") == 0 && i + 1 < argc) mobile_url = argv[++i];
            else if (strcmp(argv[i], "--alt-url") == 0 && i + 1 < argc) alt_url = argv[++i];
            else if (strcmp(argv[i], "--alt-weight") == 0 && i + 1 < argc) alt_weight = atoi(argv[++i]);
            else if (strcmp(argv[i], "--ttl") == 0 && i + 1 < argc) ttl_hours = atoi(argv[++i]);
            else if (strcmp(argv[i], "--max-clicks") == 0 && i + 1 < argc) max_clicks = atoi(argv[++i]);
            else if (strcmp(argv[i], "--key") == 0 && i + 1 < argc) api_key = argv[++i];
        }

        if (!api_key || !api_key[0]) {
            fprintf(stderr, "Error: provide --key <token> or set SHORTENER_API_KEY\n");
            return 1;
        }

        cJSON *payload = cJSON_CreateObject();
        cJSON_AddStringToObject(payload, "url", target_url);
        if (slug) cJSON_AddStringToObject(payload, "custom_slug", slug);
        if (title) cJSON_AddStringToObject(payload, "title", title);
        if (tags) cJSON_AddStringToObject(payload, "tags", tags);
        if (mobile_url) cJSON_AddStringToObject(payload, "mobile_url", mobile_url);
        if (alt_url && alt_weight > 0) {
            cJSON_AddStringToObject(payload, "alt_url", alt_url);
            cJSON_AddNumberToObject(payload, "alt_weight", alt_weight);
        }
        if (ttl_hours > 0) cJSON_AddNumberToObject(payload, "ttl_hours", ttl_hours);
        if (max_clicks > 0) cJSON_AddNumberToObject(payload, "max_clicks", max_clicks);

        char *json_str = cJSON_PrintUnformatted(payload);
        cJSON_Delete(payload);
        if (!json_str) return 1;

        int rc = http_request(host, port, "POST", "/shorten", api_key, json_str, resp, sizeof(resp), &status);
        free(json_str);
        if (rc != 0) {
            fprintf(stderr, "Error: could not connect to %s:%s\n", host, port);
            return 1;
        }

        cJSON *out = cJSON_Parse(extract_body(resp));
        if (out) {
            char *pretty = cJSON_Print(out);
            if (pretty) {
                puts(pretty);
                free(pretty);
            }
            cJSON_Delete(out);
        } else {
            puts(extract_body(resp));
        }
        return (status == 200 || status == 201) ? 0 : 1;
    }

    print_usage(argv[0]);
    return 1;
}
