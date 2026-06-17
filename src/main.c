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

#define DEFAULT_PORT 8080

// Declared in handlers.c
extern void request_completed(void *cls, struct MHD_Connection *connection,
                       void **con_cls, enum MHD_RequestTerminationCode toe);

static volatile sig_atomic_t running = 1;

static void signal_handler(int signum) {
    (void)signum;
    running = 0;
}

int main() {

    // Ensure directories exist
    mkdir("data", 0755);
    mkdir("logs", 0755);

    if (init_db("data/shortener.db") != 0) {
        fprintf(stderr, "Failed to initialize database.\n");
        return 1;
    }

    const char *port_env = getenv("PORT");
    int port = port_env && port_env[0] ? atoi(port_env) : find_available_port(DEFAULT_PORT);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid PORT value: %s\n", port_env);
        return 1;
    }
    log_message("Starting server...");
    log_message("Found available port: %d", port);
    log_message("Base URL: https://c.micutu.com");

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

    log_message("Server running on port %d (PID %d)", port, getpid());

    // Keep running until signal received
    while (running) {
        sleep(1);
    }

    // Graceful shutdown
    log_message("Shutting down gracefully...");
    MHD_stop_daemon(daemon);
    close_db();
    log_message("Server stopped.");
    return 0;
}
