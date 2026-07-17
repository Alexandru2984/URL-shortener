#include "handlers.h"
#include "db.h"
#include "utils.h"
#include <microhttpd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#define DEFAULT_PORT 8080
#define DEFAULT_BIND_ADDRESS "127.0.0.1"
#define CONNECTION_MEMORY_LIMIT (64U * 1024U)
#define CONNECTION_LIMIT 256U
#define PER_IP_CONNECTION_LIMIT 32U
#define CONNECTION_TIMEOUT_SECONDS 15U
#define MAX_BASE_URL_LEN 400U
#define MAX_API_KEY_LEN 1024U

// Defined in handlers.c
extern const char *g_api_key;
extern const char *g_base_url;

static volatile sig_atomic_t running = 1;
static char api_key_from_file[MAX_API_KEY_LEN + 1U];
static char analytics_key_from_file[MAX_API_KEY_LEN + 1U];

static int load_secret_from_file(const char *path, char *secret_out, size_t secret_out_len,
                                 const char *label) {
    if (!path || !path[0]) return -1;

    int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) {
        fprintf(stderr, "Cannot open %s file: %s\n", label, strerror(errno));
        return -1;
    }

    struct stat info;
    if (fstat(fd, &info) != 0 || !S_ISREG(info.st_mode) ||
        (info.st_mode & (S_IWGRP | S_IWOTH | S_IROTH)) != 0) {
        fprintf(stderr, "%s file must be a non-public regular file\n", label);
        close(fd);
        return -1;
    }

    ssize_t bytes_read = read(fd, secret_out, secret_out_len - 1U);
    close(fd);
    if (bytes_read <= 0 || (size_t)bytes_read == secret_out_len - 1U) {
        fprintf(stderr, "Invalid %s file\n", label);
        return -1;
    }

    size_t length = (size_t)bytes_read;
    while (length > 0 && (secret_out[length - 1U] == '\n' ||
                          secret_out[length - 1U] == '\r')) {
        length--;
    }
    if (length == 0) {
        fprintf(stderr, "%s file is empty\n", label);
        return -1;
    }
    for (size_t i = 0; i < length; i++) {
        unsigned char c = (unsigned char)secret_out[i];
        if (c < 0x21U || c > 0x7eU) {
            fprintf(stderr, "%s file contains invalid characters\n", label);
            return -1;
        }
    }
    secret_out[length] = '\0';
    return 0;
}

static int configure_secret(const char *environment_name, const char *file_environment_name,
                            char *file_buffer, size_t file_buffer_len, const char *label,
                            const char **secret_out) {
    const char *environment_value = getenv(environment_name);
    const char *file_path = getenv(file_environment_name);
    if (environment_value && environment_value[0] && file_path && file_path[0]) {
        fprintf(stderr, "Set either %s or %s, not both\n", environment_name, file_environment_name);
        return -1;
    }
    if (file_path && file_path[0]) {
        if (load_secret_from_file(file_path, file_buffer, file_buffer_len, label) != 0) return -1;
        *secret_out = file_buffer;
    } else {
        *secret_out = environment_value;
    }
    return 0;
}

static int parse_port(const char *value, uint16_t *port_out) {
    if (!value || !value[0] || !port_out) return -1;

    errno = 0;
    char *end = NULL;
    long port = strtol(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' || port < 1 || port > 65535) {
        return -1;
    }

    *port_out = (uint16_t)port;
    return 0;
}

static int ensure_private_directory(const char *path) {
    if (mkdir(path, 0750) != 0 && errno != EEXIST) {
        fprintf(stderr, "Cannot create %s: %s\n", path, strerror(errno));
        return -1;
    }

    struct stat info;
    if (lstat(path, &info) != 0 || !S_ISDIR(info.st_mode)) {
        fprintf(stderr, "%s is not a directory\n", path);
        return -1;
    }

    if (chmod(path, 0750) != 0) {
        fprintf(stderr, "Cannot secure %s: %s\n", path, strerror(errno));
        return -1;
    }
    return 0;
}

static void signal_handler(int signum) {
    (void)signum;
    running = 0;
}

// Background thread: cleanup expired links every hour
static void *cleanup_thread_func(void *arg) {
    (void)arg;
    while (running) {
        for (int i = 0; i < 3600 && running; i++) {
            sleep(1);
        }
        if (!running) break;
        int cleaned = cleanup_expired_links();
        if (cleaned > 0) {
            log_message("Cleanup: removed %d expired link(s)", cleaned);
        }
    }
    return NULL;
}

int main(void) {
    umask(0027);

    // Ensure directories exist
    if (ensure_private_directory("data") != 0 || ensure_private_directory("logs") != 0) {
        return 1;
    }

    // Read configuration from environment
    const char *port_env = getenv("PORT");
    uint16_t port = DEFAULT_PORT;
    if (port_env && parse_port(port_env, &port) != 0) {
        fprintf(stderr, "Invalid PORT value\n");
        close_db();
        return 1;
    }

    const char *bind_address = getenv("BIND_ADDRESS");
    if (!bind_address || !bind_address[0]) bind_address = DEFAULT_BIND_ADDRESS;

    struct sockaddr_in listen_address;
    memset(&listen_address, 0, sizeof(listen_address));
    listen_address.sin_family = AF_INET;
    listen_address.sin_port = htons(port);
    if (inet_pton(AF_INET, bind_address, &listen_address.sin_addr) != 1) {
        fprintf(stderr, "Invalid IPv4 BIND_ADDRESS\n");
        close_db();
        return 1;
    }

    if (configure_secret("API_KEY", "API_KEY_FILE", api_key_from_file,
                         sizeof(api_key_from_file), "API key", &g_api_key) != 0) {
        return 1;
    }
    const char *analytics_key = NULL;
    if (configure_secret("ANALYTICS_HMAC_KEY", "ANALYTICS_HMAC_KEY_FILE",
                         analytics_key_from_file, sizeof(analytics_key_from_file),
                         "analytics HMAC key", &analytics_key) != 0) {
        return 1;
    }
    const char *base_url_env = getenv("BASE_URL");
    if (base_url_env && base_url_env[0]) {
        size_t base_url_len = strnlen(base_url_env, MAX_BASE_URL_LEN + 1U);
        if (base_url_len > MAX_BASE_URL_LEN || base_url_env[base_url_len - 1U] == '/' ||
            !is_valid_http_url(base_url_env, MAX_BASE_URL_LEN)) {
            fprintf(stderr, "Invalid BASE_URL value\n");
            close_db();
            return 1;
        }
        g_base_url = base_url_env;
    }

    if (init_db("data/shortener.db", analytics_key) != 0) {
        fprintf(stderr, "Failed to initialize database.\n");
        return 1;
    }

    // Startup cleanup of expired links
    int cleaned = cleanup_expired_links();
    if (cleaned > 0) {
        log_message("Startup cleanup: removed %d expired link(s)", cleaned);
    }

    log_message("Starting server...");
    log_message("Listen: %s:%u | Base URL: %s | API Key: %s",
                bind_address, (unsigned int)port, g_base_url,
                (g_api_key && g_api_key[0]) ? "configured" : "not configured (write endpoints disabled)");
    if (g_api_key && g_api_key[0] && strnlen(g_api_key, 33U) < 32U) {
        log_error("API_KEY is shorter than 32 characters; rotate it for a stronger secret");
    }

    struct MHD_Daemon *daemon;

    // Register signal handlers before starting worker activity.
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGTERM, &sa, NULL) != 0 || sigaction(SIGINT, &sa, NULL) != 0) {
        log_error("Failed to register signal handlers");
        close_db();
        return 1;
    }

    daemon = MHD_start_daemon(MHD_USE_AUTO_INTERNAL_THREAD | MHD_USE_ERROR_LOG, 0, NULL, NULL,
                              &handle_request, NULL, 
                              MHD_OPTION_NOTIFY_COMPLETED, request_completed, NULL,
                              MHD_OPTION_SOCK_ADDR_LEN, (socklen_t)sizeof(listen_address),
                              (const struct sockaddr *)&listen_address,
                              MHD_OPTION_CONNECTION_MEMORY_LIMIT, (size_t)CONNECTION_MEMORY_LIMIT,
                              MHD_OPTION_CONNECTION_LIMIT, CONNECTION_LIMIT,
                              MHD_OPTION_PER_IP_CONNECTION_LIMIT, PER_IP_CONNECTION_LIMIT,
                              MHD_OPTION_CONNECTION_TIMEOUT, CONNECTION_TIMEOUT_SECONDS,
                              MHD_OPTION_CLIENT_DISCIPLINE_LVL, 1,
                              MHD_OPTION_END);

    if (NULL == daemon) {
        log_error("Failed to start daemon on %s:%u", bind_address, (unsigned int)port);
        close_db();
        return 1;
    }

    // Start background cleanup thread
    pthread_t cleanup_tid;
    int cleanup_started = (pthread_create(&cleanup_tid, NULL, cleanup_thread_func, NULL) == 0);
    if (!cleanup_started) {
        log_error("Failed to start cleanup thread; continuing without periodic cleanup");
    }

    log_message("Server running on %s:%u (PID %d)", bind_address, (unsigned int)port, getpid());

    // Keep running until signal received
    while (running) {
        sleep(1);
    }

    // Graceful shutdown
    log_message("Shutting down gracefully...");
    MHD_stop_daemon(daemon);
    if (cleanup_started) pthread_join(cleanup_tid, NULL);
    close_db();
    log_message("Server stopped.");
    return 0;
}
