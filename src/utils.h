#ifndef UTILS_H
#define UTILS_H

void generate_random_slug(char *slug_out, int len);
void slugify(const char *input, char *output, int max_len);
void log_message(const char *format, ...);
void log_error(const char *format, ...);
int is_port_in_use(int port);
int find_available_port(int start_port);

#endif
