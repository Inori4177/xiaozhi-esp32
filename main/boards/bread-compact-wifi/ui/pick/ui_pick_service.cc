#include "ui_pick_service.h"

#include "../laser_ui_events.h"
#include "../cnc/ui_cnc_config.h"
#include "../cnc/ui_cnc_coord_map.h"
#include "../cnc/ui_cnc_motion_facade.h"
#include "../cnc/ui_cnc_print_service.h"
#include "../../laser_ui_state.h"

#include <cmath>
#include <cstring>
#include <cstdio>
#include <esp_log.h>
#include <esp_timer.h>
#include <lvgl.h>

static const char *TAG = "ui_pick_svc";

static lv_obj_t *g_map_frame = nullptr;
static lv_obj_t *g_head_dot = nullptr;
static lv_obj_t *g_cursor_cross = nullptr;
static int g_cursor_cross_arm = 0;
static lv_obj_t *g_coord_x_label = nullptr;
static lv_obj_t *g_coord_y_label = nullptr;
static lv_obj_t *g_status_label = nullptr;

static ui_cnc_coord_viewport_t g_vp;
static float g_cursor_mm_x = 0.0f;
static float g_cursor_mm_y = 0.0f;

/* Touch drag stabilization — release commits last stable px, not noisy lift sample. */
static float g_filt_px = 0.0f;
static float g_filt_py = 0.0f;
static bool g_filt_valid = false;
static bool g_touch_active = false;
static int g_last_applied_px = -1;
static int g_last_applied_py = -1;
static int64_t g_last_drag_us = 0;

static constexpr int kDragDeadzonePx = 6;
static constexpr int64_t kMinDragIntervalUs = 40 * 1000;
static constexpr float kEmaAlpha = 0.35f;
static constexpr float kSnapMmOnRelease = 1.0f;

static esp_timer_handle_t g_pos_timer = nullptr;
static bool g_timer_running = false;

static void update_coord_label(void)
{
    char buf[24];
    if (g_coord_x_label != nullptr) {
        snprintf(buf, sizeof(buf), "X:%4.1f mm", static_cast<double>(g_cursor_mm_x));
        lv_label_set_text(g_coord_x_label, buf);
    }
    if (g_coord_y_label != nullptr) {
        snprintf(buf, sizeof(buf), "Y:%4.1f mm", static_cast<double>(g_cursor_mm_y));
        lv_label_set_text(g_coord_y_label, buf);
    }
}

static void refresh_head_dot_async(void *user_data)
{
    (void)user_data;
    if (g_head_dot == nullptr || g_map_frame == nullptr) {
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

    if (g_status_label != nullptr && !ui_cnc_print_service_is_busy()) {
        const char *txt = lv_label_get_text(g_status_label);
        if (txt != nullptr && strcmp(txt, "移动中…") == 0) {
            lv_label_set_text(g_status_label, "已就位");
            lv_obj_add_flag(g_status_label, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void pos_timer_cb(void *arg)
{
    (void)arg;
    if (ui_cnc_print_service_is_busy()) {
        return;
    }
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
    esp_timer_start_periodic(g_pos_timer, 250 * 1000);
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

static void reset_touch_filter(void)
{
    g_filt_valid = false;
    g_touch_active = false;
    g_last_applied_px = -1;
    g_last_applied_py = -1;
    g_last_drag_us = 0;
}

static void apply_cursor_px(int px, int py, bool snap_on_release)
{
    float mm_x = 0.0f;
    float mm_y = 0.0f;
    ui_cnc_px_to_mm(&g_vp, px, py, &mm_x, &mm_y);
    if (snap_on_release && kSnapMmOnRelease > 0.0f) {
        mm_x = std::round(mm_x / kSnapMmOnRelease) * kSnapMmOnRelease;
        mm_y = std::round(mm_y / kSnapMmOnRelease) * kSnapMmOnRelease;
        ui_cnc_clamp_mm(&mm_x, &mm_y);
    }
    ui_pick_service_set_cursor_mm(mm_x, mm_y);
}

static bool commit_stable_cursor(bool snap_on_release)
{
    int px = g_last_applied_px;
    int py = g_last_applied_py;
    if (px < 0 || py < 0) {
        if (!g_filt_valid) {
            return false;
        }
        px = static_cast<int>(lroundf(g_filt_px));
        py = static_cast<int>(lroundf(g_filt_py));
    }
    apply_cursor_px(px, py, snap_on_release);
    return true;
}

static void on_pick_event(laser_ui_event_id_t id, void *user_data)
{
    (void)user_data;

    switch (id) {
    case LASER_EVT_PICK_CONFIRM: {
        laser_ui_state_set_pick_origin(g_cursor_mm_x, g_cursor_mm_y);
        if (g_status_label != nullptr) {
            lv_label_set_text(g_status_label, "移动中…");
            lv_obj_remove_flag(g_status_label, LV_OBJ_FLAG_HIDDEN);
        }
        if (g_coord_x_label != nullptr || g_coord_y_label != nullptr) {
            update_coord_label();
        }
        ui_cnc_print_service_move_to_mm_async(g_cursor_mm_x, g_cursor_mm_y);
        ESP_LOGI(TAG, "Pick confirm origin=(%.1f, %.1f) mm", static_cast<double>(g_cursor_mm_x),
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
            lv_obj_add_flag(g_status_label, LV_OBJ_FLAG_HIDDEN);
        }
        ui_cnc_print_service_home_async();
        ESP_LOGI(TAG, "Pick reset to (0, 0)");
        break;
    default:
        break;
    }
}

void ui_pick_service_init(void)
{
    /* Motion facade init is lazy on first pick move (avoids early GPIO init). */
    laser_ui_events_register(on_pick_event, nullptr);
    ESP_LOGI(TAG, "initialized");
}

void ui_pick_service_on_page_show(lv_obj_t *map_frame, lv_obj_t *head_dot, lv_obj_t *cursor_cross,
                                  lv_obj_t *coord_x_label, lv_obj_t *coord_y_label,
                                  lv_obj_t *status_label)
{
    g_map_frame = map_frame;
    g_head_dot = head_dot;
    g_cursor_cross = cursor_cross;
    g_coord_x_label = coord_x_label;
    g_coord_y_label = coord_y_label;
    g_status_label = status_label;

    if (g_cursor_cross != nullptr) {
        g_cursor_cross_arm = lv_obj_get_width(g_cursor_cross) / 2;
    }

    ui_pick_service_update_viewport_from_map_frame();
    reset_touch_filter();

    if (laser_ui_state_get_pick_origin(&g_cursor_mm_x, &g_cursor_mm_y)) {
        /* restore last pick */
    } else {
        g_cursor_mm_x = UI_CNC_WORK_SIZE_MM * 0.5f;
        g_cursor_mm_y = UI_CNC_WORK_SIZE_MM * 0.5f;
    }
    update_coord_label();

    if (g_status_label != nullptr && !ui_cnc_print_service_is_busy()) {
        lv_label_set_text(g_status_label, "");
        lv_obj_add_flag(g_status_label, LV_OBJ_FLAG_HIDDEN);
    }

    start_pos_timer();
    lv_async_call(refresh_head_dot_async, nullptr);
}

void ui_pick_service_on_page_hide(void)
{
    stop_pos_timer();
    reset_touch_filter();
    g_map_frame = nullptr;
    g_head_dot = nullptr;
    g_cursor_cross = nullptr;
    g_coord_x_label = nullptr;
    g_coord_y_label = nullptr;
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

    if (g_cursor_cross != nullptr && g_cursor_cross_arm > 0) {
        int px = 0;
        int py = 0;
        ui_cnc_mm_to_px(&g_vp, x_mm, y_mm, &px, &py);
        lv_obj_set_pos(g_cursor_cross, px - g_cursor_cross_arm, py - g_cursor_cross_arm);
    }
}

void ui_pick_service_update_viewport_from_map_frame(void)
{
    if (g_map_frame == nullptr) {
        return;
    }
    ui_cnc_coord_viewport_init(&g_vp, lv_obj_get_width(g_map_frame), lv_obj_get_height(g_map_frame), 0);
}

void ui_pick_service_touch_begin(int local_x, int local_y)
{
    reset_touch_filter();
    g_touch_active = true;
    g_filt_px = static_cast<float>(local_x);
    g_filt_py = static_cast<float>(local_y);
    g_filt_valid = true;
    g_last_applied_px = local_x;
    g_last_applied_py = local_y;
    apply_cursor_px(local_x, local_y, false);
}

void ui_pick_service_set_cursor_from_local_px(int local_x, int local_y)
{
    if (!g_touch_active) {
        return;
    }
    if (!g_filt_valid) {
        ui_pick_service_touch_begin(local_x, local_y);
        return;
    }

    g_filt_px = g_filt_px * (1.0f - kEmaAlpha) + static_cast<float>(local_x) * kEmaAlpha;
    g_filt_py = g_filt_py * (1.0f - kEmaAlpha) + static_cast<float>(local_y) * kEmaAlpha;

    const int px = static_cast<int>(lroundf(g_filt_px));
    const int py = static_cast<int>(lroundf(g_filt_py));

    if (g_last_applied_px >= 0) {
        const int dx = px - g_last_applied_px;
        const int dy = py - g_last_applied_py;
        if ((dx * dx + dy * dy) < (kDragDeadzonePx * kDragDeadzonePx)) {
            return;
        }
        const int64_t now = esp_timer_get_time();
        if ((now - g_last_drag_us) < kMinDragIntervalUs) {
            return;
        }
        g_last_drag_us = now;
    }

    g_last_applied_px = px;
    g_last_applied_py = py;
    apply_cursor_px(px, py, false);
}

void ui_pick_service_touch_end(int local_x, int local_y)
{
    (void)local_x;
    (void)local_y;
    if (!g_touch_active) {
        return;
    }
    g_touch_active = false;
    /* Do not use lift-off coordinates — they jitter; lock last drag position. */
    commit_stable_cursor(true);
    reset_touch_filter();
}
