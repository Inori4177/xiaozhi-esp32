#pragma once

#include "peer_uart_link.h"

#include <cstdio>

#ifdef __cplusplus
extern "C" {
#endif

static inline bool peer_voice_tx_chat(const char *role, const char *text)
{
    if (text == nullptr) {
        return false;
    }
    char buf[512];
    snprintf(buf, sizeof(buf), "{\"t\":\"chat\",\"role\":\"%s\",\"text\":\"%s\"}",
             role ? role : "assistant", text);
    return peer_uart_link_send_line(buf);
}

static inline bool peer_voice_tx_emotion(const char *id)
{
    if (id == nullptr) {
        return false;
    }
    char buf[128];
    snprintf(buf, sizeof(buf), "{\"t\":\"emotion\",\"id\":\"%s\"}", id);
    return peer_uart_link_send_line(buf);
}

static inline bool peer_voice_tx_state(const char *state)
{
    if (state == nullptr) {
        return false;
    }
    char buf[128];
    snprintf(buf, sizeof(buf), "{\"t\":\"voice_state\",\"state\":\"%s\"}", state);
    return peer_uart_link_send_line(buf);
}

#ifdef __cplusplus
}
#endif
