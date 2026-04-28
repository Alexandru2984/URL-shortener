#include "handlers.h"
#include "db.h"
#include "utils.h"
#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#define DEFAULT_PORT 8080

// Need to declare request_completed
extern void request_completed(void *cls, struct MHD_Connection *connection,
                       void **con_cls, enum MHD_RequestTerminationCode toe);

int main() {
    srand(time(NULL));

    // Ensure directories exist
    mkdir("data", 0755);
    mkdir("logs", 0755);

    if (init_db("data/shortener.db") != 0) {
        fprintf(stderr, "Failed to initialize database.\n");
        return 1;
    }

    int port = find_available_port(DEFAULT_PORT);
    log_message("Starting server...");
    log_message("Found available port: %d", port);
    log_message("Base URL: http://c.micutu.com");

    struct MHD_Daemon *daemon;

    daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG, port, NULL, NULL,
                              &handle_request, NULL, 
                              MHD_OPTION_NOTIFY_COMPLETED, request_completed, NULL,
                              MHD_OPTION_END);

    if (NULL == daemon) {
        log_error("Failed to start daemon on port %d", port);
        return 1;
    }

    // Keep running
    while (1) {
        sleep(60);
    }

    MHD_stop_daemon(daemon);
    return 0;
}
