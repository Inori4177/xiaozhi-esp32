#include "laser_ui_log.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include <cstring>
#include <stdio.h>

static const char *TAG = "laser_ui_log";

static constexpr int kMaxEntries = 32;
static constexpr int kEntryLen = 256;
static constexpr int kTextBufLen = 8192;

struct LogEntry {
    char role[16];
    char content[kEntryLen];
};

static LogEntry g_entries[kMaxEntries];
static int g_count = 0;
static int g_head = 0;
static char g_text_buf[kTextBufLen];
static SemaphoreHandle_t g_mutex = nullptr;
static laser_ui_log_refresh_cb_t g_refresh_cb = nullptr;

static void rebuild_text_locked(void)
{
    g_text_buf[0] = '\0';
    size_t offset = 0;
    int start = (g_count < kMaxEntries) ? 0 : g_head;
    int total = (g_count < kMaxEntries) ? g_count : kMaxEntries;

    for (int i = 0; i < total; ++i) {
        int idx = (start + i) % kMaxEntries;
        int written = snprintf(g_text_buf + offset, kTextBufLen - offset,
                               "[%s] %s\n", g_entries[idx].role, g_entries[idx].content);
        if (written <= 0 || offset + written >= kTextBufLen - 1) {
            break;
        }
        offset += written;
    }
}

void laser_ui_log_init(void)
{
    if (g_mutex == nullptr) {
        g_mutex = xSemaphoreCreateMutex();
    }
    if (g_mutex == nullptr) {
        ESP_LOGE(TAG, "Failed to create log mutex");
        return;
    }
    xSemaphoreTake(g_mutex, portMAX_DELAY);
    g_count = 0;
    g_head = 0;
    g_text_buf[0] = '\0';
    xSemaphoreGive(g_mutex);
}

void laser_ui_log_set_refresh_cb(laser_ui_log_refresh_cb_t cb)
{
    g_refresh_cb = cb;
}

void laser_ui_log_append(const char *role, const char *content)
{
    if (role == nullptr || content == nullptr || content[0] == '\0') {
        return;
    }
    if (g_mutex == nullptr) {
        laser_ui_log_init();
    }
    if (g_mutex == nullptr) {
        return;
    }

    xSemaphoreTake(g_mutex, portMAX_DELAY);
    int idx = (g_head + g_count) % kMaxEntries;
    if (g_count >= kMaxEntries) {
        g_head = (g_head + 1) % kMaxEntries;
        idx = (g_head + kMaxEntries - 1) % kMaxEntries;
    } else {
        g_count++;
    }

    strncpy(g_entries[idx].role, role, sizeof(g_entries[idx].role) - 1);
    g_entries[idx].role[sizeof(g_entries[idx].role) - 1] = '\0';
    strncpy(g_entries[idx].content, content, sizeof(g_entries[idx].content) - 1);
    g_entries[idx].content[sizeof(g_entries[idx].content) - 1] = '\0';

    rebuild_text_locked();
    xSemaphoreGive(g_mutex);

    if (g_refresh_cb != nullptr) {
        g_refresh_cb();
    }
}

const char *laser_ui_log_get_text(void)
{
    return g_text_buf;
}
