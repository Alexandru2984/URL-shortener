#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>

void slugify(const char *input, char *output, int max_len) {
    int i = 0, j = 0;
    while (input[i] && j < max_len - 1) {
        char c = input[i];
        if (isalnum((unsigned char)c)) {
            output[j++] = tolower((unsigned char)c);
        } else if (c == ' ' || c == '-' || c == '_') {
            if (j > 0 && output[j-1] != '-') {
                output[j++] = '-';
            }
        }
        i++;
    }
    if (j > 0 && output[j-1] == '-') j--;
    output[j] = '\0';
}

void generate_random_slug(char *slug_out, int len) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < len; i++) {
        int key = rand() % (int)(sizeof(charset) - 1);
        slug_out[i] = charset[key];
    }
    slug_out[len] = '\0';
}

static void log_to_file(const char *filename, const char *level, const char *format, va_list args) {
    FILE *f = fopen(filename, "a");
    if (!f) return;
    
    time_t now;
    time(&now);
    struct tm *tm_info = localtime(&now);
    char time_buf[26];
    strftime(time_buf, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    
    fprintf(f, "[%s] [%s] ", time_buf, level);
    vfprintf(f, format, args);
    fprintf(f, "\n");
    fclose(f);
}

void log_message(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_to_file("logs/access.log", "INFO", format, args);
    va_end(args);
    
    // Also print to stdout
    va_start(args, format);
    printf("[INFO] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void log_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_to_file("logs/error.log", "ERROR", format, args);
    va_end(args);
    
    // Also print to stderr
    va_start(args, format);
    fprintf(stderr, "[ERROR] ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

int is_port_in_use(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return 1; // Assume in use if we can't create socket
    
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    
    int result = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    close(sockfd);
    
    if (result < 0) return 1; // Port in use
    return 0; // Port available
}

int find_available_port(int start_port) {
    int port = start_port;
    while (is_port_in_use(port) && port < 65535) {
        port++;
    }
    return port;
}
