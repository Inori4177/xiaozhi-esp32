#include "ui_pick_service.h"

#include "../laser_ui_events.h"
#include "../cnc/ui_cnc_config.h"
#include "../cnc/ui_cnc_coord_map.h"
#include "../cnc/ui_cnc_motion_facade.h"
#include "../../laser_ui_state.h"

#include <cstring>
#include <cstdio>
#include <esp_log.h>
#include <esp_timer.h>
#include <lvgl.h>

static const char *TAG = "ui_pick_svc";

static lv_obj_t *g_work_area = nullptr;
static lv_obj_t *g_head_dot = nullptr;
static lv_obj_t *g_cursor_dot = nullptr;
static lv_obj_t *g_coord_label = nullptr;
static lv_obj_t *g_status_label = nullptr;

static ui_cnc_coord_viewport_t g_vp;
static float g_cursor_mm_x = 0.0f;
static float g_cursor_mm_y = 0.0f;

static esp_timer_handle_t g_pos_timer = nullptr;
static bool g_timer_running = false;

static void update_coord_label(void)
{
    if (g_coord_label == nullptr) {
        return;
    }
    char buf[48];
    snprintf(buf, sizeof(buf), "光标 X:%.1f  Y:%.1f mm", static_cast<double>(g_cursor_mm_x),
             static_cast<double>(g_cursor_mm_y));
    lv_label_set_text(g_coord_label, buf);
}

static void refresh_head_dot_async(void *user_data)
{
    (void)user_data;
    if (g_head_dot == nullptr || g_work_area == nullptr) {
        return;
    }

    float hx = 0.0f;
    float hy = 0.0f;
    ui_cnc_motion_facade_get_position_mm(&hx, &hy);

    int px = 0;
    int py = 0;
    ui_cnc_mm_to_px(&g_vp, hx, hy, &px, &py);

    const int dot_r = lv_obj_get_width(g_head_dot) / 2;
    lv_obj_set_pos(g_head_dot, px - dot_r, py - dot_r);

    if (g_status_label != nullptr && !ui_cnc_motion_facade_is_moving()) {
        const char *txt = lv_label_get_text(g_status_label);
        if (txt != nullptr && strcmp(txt, "移动中…") == 0) {
            lv_label_set_text(g_status_label, "已就位");
        }
    }
}

static void pos_timer_cb(void *arg)
{
    (void)arg;
    lv_async_call(refresh_head_dot_async, nullptr);
}

static void start_pos_timer(void)
{
    if (g_timer_running) {
        return;
    }
    if (g_pos_timer == nullptr) {
        const esp_timer_create_args_t args = {
            .callback = &pos_timer_cb,
            .arg = nullptr,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "ui_pick_pos",
            .skip_unhandled_events = false,
        };
        esp_timer_create(&args, &g_pos_timer);
    }
    esp_timer_start_periodic(g_pos_timer, 100 * 1000);
    g_timer_running = true;
}

static void stop_pos_timer(void)
{
    if (!g_timer_running || g_pos_timer == nullptr) {
        return;
    }
    esp_timer_stop(g_pos_timer);
    g_timer_running = false;
}

static void on_pick_event(laser_ui_event_id_t id, void *user_data)
{
    (void)user_data;

    switch (id) {
    case LASER_EVT_PICK_CONFIRM: {
        laser_ui_state_set_pick_origin(g_cursor_mm_x, g_cursor_mm_y);
        if (g_status_label != nullptr) {
            lv_label_set_text(g_status_label, "移动中…");
        }
        ui_cnc_motion_facade_rapid_to_mm_async(g_cursor_mm_x, g_cursor_mm_y);
        ESP_LOGI(TAG, "Pick confirm (%.2f, %.2f)", static_cast<double>(g_cursor_mm_x),
                 static_cast<double>(g_cursor_mm_y));
        break;
    }
    case LASER_EVT_PICK_RESET:
        g_cursor_mm_x = 0.0f;
        g_cursor_mm_y = 0.0f;
        laser_ui_state_clear_pick_origin();
        update_coord_label();
        if (g_status_label != nullptr) {
            lv_label_set_text(g_status_label, "");
        }
        ui_cnc_motion_facade_rapid_to_mm_async(0.0f, 0.0f);
        ESP_LOGI(TAG, "Pick reset to origin");
        break;
    default:
        break;
    }
}

void ui_pick_service_init(void)
{
    /* Do not call ui_cnc_motion_facade_init() here: SteppingEngine GPIOs overlap
     * LCD SPI / touch on bread-compact-wifi; init runs lazily on first pick move. */
    laser_ui_events_register(on_pick_event, nullptr);
    ESP_LOGI(TAG, "initialized");
}

void ui_pick_service_on_page_show(lv_obj_t *work_area, lv_obj_t *head_dot, lv_obj_t *cursor_dot,
                                  lv_obj_t *coord_label, lv_obj_t *status_label)
{
    g_work_area = work_area;
    g_head_dot = head_dot;
    g_cursor_dot = cursor_dot;
    g_coord_label = coord_label;
    g_status_label = status_label;

    if (g_work_area != nullptr) {
        const int w = lv_obj_get_width(g_work_area);
        const int h = lv_obj_get_height(g_work_area);
        ui_cnc_coord_viewport_init(&g_vp, w, h, 8);
    }

    if (laser_ui_state_get_pick_origin(&g_cursor_mm_x, &g_cursor_mm_y)) {
        /* restore last pick */
    } else {
        g_cursor_mm_x = UI_CNC_WORK_SIZE_MM * 0.5f;
        g_cursor_mm_y = UI_CNC_WORK_SIZE_MM * 0.5f;
    }
    update_coord_label();

    if (g_status_label != nullptr && !ui_cnc_motion_facade_is_moving()) {
        lv_label_set_text(g_status_label, "");
    }

    start_pos_timer();
    lv_async_call(refresh_head_dot_async, nullptr);
}

void ui_pick_service_on_page_hide(void)
{
    stop_pos_timer();
    g_work_area = nullptr;
    g_head_dot = nullptr;
    g_cursor_dot = nullptr;
    g_coord_label = nullptr;
    g_status_label = nullptr;
}

void ui_pick_service_get_cursor_mm(float *x_mm, float *y_mm)
{
    if (x_mm != nullptr) {
        *x_mm = g_cursor_mm_x;
    }
    if (y_mm != nullptr) {
        *y_mm = g_cursor_mm_y;
    }
}

/** Called from page_pick when cursor moves. */
void ui_pick_service_set_cursor_mm(float x_mm, float y_mm)
{
    g_cursor_mm_x = x_mm;
    g_cursor_mm_y = y_mm;
    update_coord_label();

    if (g_cursor_dot != nullptr) {
        int px = 0;
        int py = 0;
        ui_cnc_mm_to_px(&g_vp, x_mm, y_mm, &px, &py);
        const int r = lv_obj_get_width(g_cursor_dot) / 2;
        lv_obj_set_pos(g_cursor_dot, px - r, py - r);
    }
}

void ui_pick_service_update_viewport_from_work_area(void)
{
    if (g_work_area == nullptr) {
        return;
    }
    ui_cnc_coord_viewport_init(&g_vp, lv_obj_get_width(g_work_area), lv_obj_get_height(g_work_area), 8);
}

void ui_pick_service_set_cursor_from_local_px(int local_x, int local_y)
{
    float mm_x = 0.0f;
    float mm_y = 0.0f;
    ui_cnc_px_to_mm(&g_vp, local_x, local_y, &mm_x, &mm_y);
    ui_pick_service_set_cursor_mm(mm_x, mm_y);
}
