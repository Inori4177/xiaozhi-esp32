#pragma once

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*peer_uart_line_cb_t)(const char *line, void *user_data);

bool peer_uart_link_init(void);
bool peer_uart_link_is_up(void);
void peer_uart_link_set_line_callback(peer_uart_line_cb_t cb, void *user_data);
bool peer_uart_link_send_line(const char *line);

#ifdef __cplusplus
}
#endif
