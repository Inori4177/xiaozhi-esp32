#include "webui_log.h"
#include "webui_config.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static char s_ring[WEBUI_LOG_RING_SIZE];
static size_t s_head = 0;
static SemaphoreHandle_t s_mux = nullptr;

void webui_log_init(void)
{
    if (s_mux == nullptr) {
        s_mux = xSemaphoreCreateMutex();
    }
    s_head = 0;
    s_ring[0] = '\0';
}

void webui_log_append(const char *line)
{
    if (line == nullptr || s_mux == nullptr) {
        return;
    }
    if (xSemaphoreTake(s_mux, pdMS_TO_TICKS(50)) != pdTRUE) {
        return;
    }
    const size_t len = strlen(line);
    for (size_t i = 0; i < len; ++i) {
        s_ring[s_head] = line[i];
        s_head = (s_head + 1) % WEBUI_LOG_RING_SIZE;
    }
    if (len > 0 && line[len - 1] != '\n') {
        s_ring[s_head] = '\n';
        s_head = (s_head + 1) % WEBUI_LOG_RING_SIZE;
    }
    xSemaphoreGive(s_mux);
}

void webui_log_appendf(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    webui_log_append(buf);
}

size_t webui_log_snapshot(char *buf, size_t buf_size)
{
    if (buf == nullptr || buf_size == 0 || s_mux == nullptr) {
        return 0;
    }
    if (xSemaphoreTake(s_mux, pdMS_TO_TICKS(50)) != pdTRUE) {
        return 0;
    }
    const size_t cap = buf_size - 1;
    size_t out = 0;
    size_t pos = s_head;
    while (out < cap) {
        const char c = s_ring[pos];
        if (c == '\0' && pos == s_head) {
            break;
        }
        buf[out++] = c;
        pos = (pos + 1) % WEBUI_LOG_RING_SIZE;
        if (pos == s_head && out > 0) {
            break;
        }
    }
    buf[out] = '\0';
    xSemaphoreGive(s_mux);
    return out;
}
