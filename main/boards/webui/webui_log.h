#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void webui_log_init(void);
void webui_log_append(const char *line);
void webui_log_appendf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/** Copy recent log into buf (NUL-terminated). Returns bytes written. */
size_t webui_log_snapshot(char *buf, size_t buf_size);

#ifdef __cplusplus
}
#endif
