#include "laser_ui_events.h"

#include <esp_log.h>

static const char *TAG = "laser_ui_evt";

static laser_ui_event_handler_t g_handler = nullptr;
static void *g_handler_user = nullptr;

void laser_ui_events_init(void)
{
    g_handler = nullptr;
    g_handler_user = nullptr;
}

void laser_ui_events_register(laser_ui_event_handler_t handler, void *user_data)
{
    g_handler = handler;
    g_handler_user = user_data;
}

void laser_ui_events_emit(laser_ui_event_id_t id)
{
    if (g_handler != nullptr) {
        g_handler(id, g_handler_user);
        return;
    }
    ESP_LOGD(TAG, "event %d (stub, not implemented)", static_cast<int>(id));
}
