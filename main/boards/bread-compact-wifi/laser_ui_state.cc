#include "laser_ui_state.h"

#include <cstdio>

static lv_obj_t *g_step_label = nullptr;
static lv_obj_t *g_step_slider = nullptr;
static lv_obj_t *g_material_dd = nullptr;
static lv_obj_t *g_power_slider = nullptr;
static lv_obj_t *g_power_label = nullptr;
static lv_obj_t *g_speed_slider = nullptr;
static lv_obj_t *g_speed_label = nullptr;

static const float k_step_mm[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
static const int k_step_count = sizeof(k_step_mm) / sizeof(k_step_mm[0]);

/** Power slider index 0..10 → 0%..100% in 10% steps. */
static const int k_power_index_max = 10;

/** Speed slider index 0..5 → 50%..300% in 50% steps. */
static const int k_speed_pct[] = {50, 100, 150, 200, 250, 300};
static const int k_speed_count = sizeof(k_speed_pct) / sizeof(k_speed_pct[0]);
static const int k_speed_index_max = k_speed_count - 1;

static float g_pick_origin_x = 0.0f;
static float g_pick_origin_y = 0.0f;
static bool g_pick_origin_valid = false;

static laser_ui_settings_t g_cached_settings = {
    .material_index = 0,
    .laser_power_pct = 50,
    .speed_pct = 100,
};
static float g_cached_jog_step_mm = 1.0f;

static int clamp_index(int idx, int max_index)
{
    if (idx < 0) {
        return 0;
    }
    if (idx > max_index) {
        return max_index;
    }
    return idx;
}

static int snap_slider_index(lv_obj_t *slider, int max_index)
{
    if (slider == nullptr) {
        return 0;
    }
    int idx = clamp_index(static_cast<int>(lv_slider_get_value(slider)), max_index);
    if (static_cast<int>(lv_slider_get_value(slider)) != idx) {
        lv_slider_set_value(slider, idx, LV_ANIM_OFF);
    }
    return idx;
}

static int clamp_step_index(int idx)
{
    return clamp_index(idx, k_step_count - 1);
}

static void refresh_settings_cache(void)
{
    if (g_material_dd != nullptr) {
        g_cached_settings.material_index = static_cast<int>(lv_dropdown_get_selected(g_material_dd));
    }
    if (g_power_slider != nullptr) {
        const int idx = snap_slider_index(g_power_slider, k_power_index_max);
        g_cached_settings.laser_power_pct = idx * 10;
    }
    if (g_speed_slider != nullptr) {
        const int idx = snap_slider_index(g_speed_slider, k_speed_index_max);
        g_cached_settings.speed_pct = k_speed_pct[idx];
    }
}

static void refresh_jog_step_cache(void)
{
    if (g_step_slider == nullptr) {
        g_cached_jog_step_mm = 1.0f;
        return;
    }
    const int idx = clamp_step_index(static_cast<int>(lv_slider_get_value(g_step_slider)));
    g_cached_jog_step_mm = k_step_mm[idx];
}

static void update_step_label(void)
{
    if (g_step_label == nullptr || g_step_slider == nullptr) {
        return;
    }
    int idx = clamp_step_index(static_cast<int>(lv_slider_get_value(g_step_slider)));
    char buf[24];
    snprintf(buf, sizeof(buf), "%dmm", static_cast<int>(k_step_mm[idx]));
    lv_label_set_text(g_step_label, buf);
    refresh_jog_step_cache();
}

static void update_power_label(void)
{
    if (g_power_label == nullptr || g_power_slider == nullptr) {
        return;
    }
    int idx = snap_slider_index(g_power_slider, k_power_index_max);
    char buf[12];
    snprintf(buf, sizeof(buf), "%d%%", idx * 10);
    lv_label_set_text(g_power_label, buf);
    refresh_settings_cache();
}

static void update_speed_label(void)
{
    if (g_speed_label == nullptr || g_speed_slider == nullptr) {
        return;
    }
    int idx = snap_slider_index(g_speed_slider, k_speed_index_max);
    char buf[12];
    snprintf(buf, sizeof(buf), "%d%%", k_speed_pct[idx]);
    lv_label_set_text(g_speed_label, buf);
    refresh_settings_cache();
}

void laser_ui_state_init(void)
{
    g_step_label = nullptr;
    g_step_slider = nullptr;
    g_material_dd = nullptr;
    g_power_slider = nullptr;
    g_power_label = nullptr;
    g_speed_slider = nullptr;
    g_speed_label = nullptr;
}

void laser_ui_state_bind_print(lv_obj_t *step_label, lv_obj_t *step_slider)
{
    g_step_label = step_label;
    g_step_slider = step_slider;
    update_step_label();
    refresh_jog_step_cache();
}

void laser_ui_state_bind_settings(lv_obj_t *material_dd, lv_obj_t *power_slider,
                                  lv_obj_t *power_label, lv_obj_t *speed_slider,
                                  lv_obj_t *speed_label)
{
    g_material_dd = material_dd;
    g_power_slider = power_slider;
    g_power_label = power_label;
    g_speed_slider = speed_slider;
    g_speed_label = speed_label;
    update_power_label();
    update_speed_label();
    refresh_settings_cache();
}

void laser_ui_state_on_step_slider(lv_event_t *e)
{
    (void)e;
    update_step_label();
}

void laser_ui_state_on_power_slider(lv_event_t *e)
{
    if (e == nullptr || lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    update_power_label();
}

void laser_ui_state_on_speed_slider(lv_event_t *e)
{
    if (e == nullptr || lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    update_speed_label();
}

void laser_ui_state_on_material_changed(lv_event_t *e)
{
    (void)e;
    refresh_settings_cache();
}

float laser_ui_state_get_jog_step_mm(void)
{
    return g_cached_jog_step_mm;
}

laser_ui_settings_t laser_ui_state_get_settings(void)
{
    return g_cached_settings;
}

void laser_ui_state_set_pick_origin(float x_mm, float y_mm)
{
    g_pick_origin_x = x_mm;
    g_pick_origin_y = y_mm;
    g_pick_origin_valid = true;
}

bool laser_ui_state_get_pick_origin(float *x_mm, float *y_mm)
{
    if (!g_pick_origin_valid) {
        return false;
    }
    if (x_mm != nullptr) {
        *x_mm = g_pick_origin_x;
    }
    if (y_mm != nullptr) {
        *y_mm = g_pick_origin_y;
    }
    return true;
}

void laser_ui_state_clear_pick_origin(void)
{
    g_pick_origin_valid = false;
}
