#pragma once

#include "peer_uart_link.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Register an additional RX listener (up to 3 extra besides primary callback). */
bool peer_uart_link_add_listener(peer_uart_line_cb_t cb, void *user_data);

#ifdef __cplusplus
}
#endif
