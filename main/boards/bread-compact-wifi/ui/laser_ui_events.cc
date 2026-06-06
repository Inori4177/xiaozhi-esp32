#include "laser_ui_events.h"

#include "cnc/ui_cnc_config.h"
#include "cnc/ui_cnc_print_service.h"

#include <lvgl.h>

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

static const char *TAG = "laser_ui_evt";

static constexpr int kMaxHandlers = 4;
static constexpr int kEvtQueueLen = 16;
static constexpr int kEvtTaskStack = 3072;

static laser_ui_event_handler_t g_handlers[kMaxHandlers] = {};
static void *g_handler_users[kMaxHandlers] = {};
static int g_handler_count = 0;

static QueueHandle_t s_evt_queue = nullptr;

const char *laser_ui_event_name(laser_ui_event_id_t id)
{
    switch (id) {
    case LASER_EVT_NAV_SWITCH:
        return "nav_switch";
    case LASER_EVT_JOG_X_PLUS:
        return "jog_x+";
    case LASER_EVT_JOG_X_MINUS:
        return "jog_x-";
    case LASER_EVT_JOG_Y_PLUS:
        return "jog_y+";
    case LASER_EVT_JOG_Y_MINUS:
        return "jog_y-";
    case LASER_EVT_JOG_HOME:
        return "jog_home";
    case LASER_EVT_RUN:
        return "run";
    case LASER_EVT_PAUSE:
        return "pause";
    case LASER_EVT_STEP_CHANGED:
        return "step_changed";
    case LASER_EVT_MATERIAL_CHANGED:
        return "material_changed";
    case LASER_EVT_POWER_CHANGED:
        return "power_changed";
    case LASER_EVT_SPEED_CHANGED:
        return "speed_changed";
    case LASER_EVT_SETTINGS_APPLY:
        return "settings_apply";
    case LASER_EVT_PICK_CONFIRM:
        return "pick_confirm";
    case LASER_EVT_PICK_RESET:
        return "pick_reset";
    default:
        return "unknown";
    }
}

static bool is_cnc_event(laser_ui_event_id_t id)
{
    switch (id) {
    case LASER_EVT_JOG_X_PLUS:
    case LASER_EVT_JOG_X_MINUS:
    case LASER_EVT_JOG_Y_PLUS:
    case LASER_EVT_JOG_Y_MINUS:
    case LASER_EVT_JOG_HOME:
    case LASER_EVT_RUN:
    case LASER_EVT_PAUSE:
    case LASER_EVT_STEP_CHANGED:
    case LASER_EVT_MATERIAL_CHANGED:
    case LASER_EVT_POWER_CHANGED:
    case LASER_EVT_SPEED_CHANGED:
    case LASER_EVT_SETTINGS_APPLY:
        return true;
    default:
        return false;
    }
}

static void dispatch_lvgl_handlers(void *user_data)
{
    const auto id = static_cast<laser_ui_event_id_t>(reinterpret_cast<intptr_t>(user_data));
    for (int i = 0; i < g_handler_count; ++i) {
        if (g_handlers[i] != nullptr) {
            g_handlers[i](id, g_handler_users[i]);
        }
    }
}

/** RUN/PAUSE 在独立高优先级任务执行，避免与 ui_cnc_worker 同核优先级反转导致暂停无效。 */
static void cnc_transport_invoke_task(void *param)
{
    const auto id = static_cast<laser_ui_event_id_t>(reinterpret_cast<intptr_t>(param));
    ui_cnc_print_service_on_event(id);
    vTaskDelete(nullptr);
}

static void dispatch_cnc_transport(laser_ui_event_id_t id)
{
    if (xTaskCreatePinnedToCore(cnc_transport_invoke_task, "cnc_transport", 2560,
                                reinterpret_cast<void *>(static_cast<intptr_t>(id)), 6, nullptr,
                                1) != pdPASS) {
        ESP_LOGW(TAG, "cnc_transport task create failed, inline dispatch");
        ui_cnc_print_service_on_event(id);
    }
}

static void ui_evt_task(void *arg)
{
    (void)arg;
    laser_ui_event_id_t id;

    for (;;) {
        if (xQueueReceive(s_evt_queue, &id, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        if (id == LASER_EVT_RUN || id == LASER_EVT_PAUSE) {
            ESP_LOGI(TAG, "dispatch transport: %s", laser_ui_event_name(id));
            dispatch_cnc_transport(id);
            continue;
        }

        if (is_cnc_event(id)) {
            ESP_LOGI(TAG, "dispatch cnc: %s", laser_ui_event_name(id));
            ui_cnc_print_service_on_event(id);
            continue;
        }

        if (g_handler_count == 0) {
            ESP_LOGW(TAG, "event %s (%d) no handler", laser_ui_event_name(id), static_cast<int>(id));
            continue;
        }

        ESP_LOGI(TAG, "dispatch lvgl handler: %s", laser_ui_event_name(id));
        if (lv_async_call(dispatch_lvgl_handlers, reinterpret_cast<void *>(static_cast<intptr_t>(id))) !=
            LV_RESULT_OK) {
            ESP_LOGW(TAG, "lv_async_call failed for %s", laser_ui_event_name(id));
        }
    }
}

void laser_ui_events_init(void)
{
    g_handler_count = 0;

    if (s_evt_queue != nullptr) {
        return;
    }

    s_evt_queue = xQueueCreate(kEvtQueueLen, sizeof(laser_ui_event_id_t));
    if (s_evt_queue == nullptr) {
        ESP_LOGE(TAG, "failed to create evt queue");
        return;
    }

    TaskHandle_t evt_task = nullptr;
    if (xTaskCreatePinnedToCore(ui_evt_task, "ui_evt", kEvtTaskStack, nullptr, 3, &evt_task,
                                UI_CNC_TASK_CORE) != pdPASS) {
        ESP_LOGE(TAG, "failed to create evt task");
        vQueueDelete(s_evt_queue);
        s_evt_queue = nullptr;
    }
}

void laser_ui_events_register(laser_ui_event_handler_t handler, void *user_data)
{
    if (handler == nullptr) {
        return;
    }
    for (int i = 0; i < g_handler_count; ++i) {
        if (g_handlers[i] == handler) {
            g_handler_users[i] = user_data;
            return;
        }
    }
    if (g_handler_count >= kMaxHandlers) {
        ESP_LOGW(TAG, "handler table full, drop register");
        return;
    }
    g_handlers[g_handler_count] = handler;
    g_handler_users[g_handler_count] = user_data;
    g_handler_count++;
}

void laser_ui_events_emit(laser_ui_event_id_t id)
{
    if (s_evt_queue == nullptr) {
        ESP_LOGW(TAG, "emit %d before init", static_cast<int>(id));
        return;
    }
    if (xQueueSend(s_evt_queue, &id, 0) != pdTRUE) {
        ESP_LOGW(TAG, "evt queue full, drop %s", laser_ui_event_name(id));
        return;
    }
    ESP_LOGI(TAG, "queued %s", laser_ui_event_name(id));
}
