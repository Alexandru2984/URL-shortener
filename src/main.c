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

// Defined in handlers.c
extern const char *g_api_key;
extern const char *g_base_url;

static volatile sig_atomic_t running = 1;

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

    if (init_db("data/shortener.db") != 0) {
        fprintf(stderr, "Failed to initialize database.\n");
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

    g_api_key = getenv("API_KEY");
    const char *base_url_env = getenv("BASE_URL");
    if (base_url_env && base_url_env[0]) {
        g_base_url = base_url_env;
    }

    // Startup cleanup of expired links
    int cleaned = cleanup_expired_links();
    if (cleaned > 0) {
        log_message("Startup cleanup: removed %d expired link(s)", cleaned);
    }

    log_message("Starting server...");
    log_message("Listen: %s:%u | Base URL: %s | API Key: %s",
                bind_address, (unsigned int)port, g_base_url,
                (g_api_key && g_api_key[0]) ? "configured" : "disabled (open access)");

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
