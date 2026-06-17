#include "handlers.h"
#include "db.h"
#include "utils.h"
#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#define DEFAULT_PORT 8080

// Defined in handlers.c
extern const char *g_api_key;
extern const char *g_base_url;

static volatile sig_atomic_t running = 1;

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

int main() {

    // Ensure directories exist
    mkdir("data", 0755);
    mkdir("logs", 0755);

    if (init_db("data/shortener.db") != 0) {
        fprintf(stderr, "Failed to initialize database.\n");
        return 1;
    }

    // Read configuration from environment
    const char *port_env = getenv("PORT");
    int port = port_env && port_env[0] ? atoi(port_env) : find_available_port(DEFAULT_PORT);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid PORT value: %s\n", port_env);
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
    log_message("Port: %d | Base URL: %s | API Key: %s",
                port, g_base_url,
                (g_api_key && g_api_key[0]) ? "configured" : "disabled (open access)");

    struct MHD_Daemon *daemon;

    daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG, port, NULL, NULL,
                              &handle_request, NULL, 
                              MHD_OPTION_NOTIFY_COMPLETED, request_completed, NULL,
                              MHD_OPTION_END);

    if (NULL == daemon) {
        log_error("Failed to start daemon on port %d", port);
        return 1;
    }

    // Register signal handlers for graceful shutdown
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    // Start background cleanup thread
    pthread_t cleanup_tid;
    pthread_create(&cleanup_tid, NULL, cleanup_thread_func, NULL);

    log_message("Server running on port %d (PID %d)", port, getpid());

    // Keep running until signal received
    while (running) {
        sleep(1);
    }

    // Graceful shutdown
    log_message("Shutting down gracefully...");
    MHD_stop_daemon(daemon);
    pthread_join(cleanup_tid, NULL);
    close_db();
    log_message("Server stopped.");
    return 0;
}
