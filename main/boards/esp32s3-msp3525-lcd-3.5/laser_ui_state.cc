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

static int clamp_step_index(int idx)
{
    if (idx < 0) {
        return 0;
    }
    if (idx >= k_step_count) {
        return k_step_count - 1;
    }
    return idx;
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
}

static void update_power_label(void)
{
    if (g_power_label == nullptr || g_power_slider == nullptr) {
        return;
    }
    int pct = static_cast<int>(lv_slider_get_value(g_power_slider));
    char buf[12];
    snprintf(buf, sizeof(buf), "%d%%", pct);
    lv_label_set_text(g_power_label, buf);
}

static void update_speed_label(void)
{
    if (g_speed_label == nullptr || g_speed_slider == nullptr) {
        return;
    }
    int pct = static_cast<int>(lv_slider_get_value(g_speed_slider));
    char buf[12];
    snprintf(buf, sizeof(buf), "%d%%", pct);
    lv_label_set_text(g_speed_label, buf);
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
}

void laser_ui_state_on_step_slider(lv_event_t *e)
{
    (void)e;
    update_step_label();
}

void laser_ui_state_on_power_slider(lv_event_t *e)
{
    (void)e;
    update_power_label();
}

void laser_ui_state_on_speed_slider(lv_event_t *e)
{
    (void)e;
    update_speed_label();
}

void laser_ui_state_on_material_changed(lv_event_t *e)
{
    (void)e;
}

float laser_ui_state_get_jog_step_mm(void)
{
    if (g_step_slider == nullptr) {
        return 1.0f;
    }
    int idx = clamp_step_index(static_cast<int>(lv_slider_get_value(g_step_slider)));
    return k_step_mm[idx];
}

laser_ui_settings_t laser_ui_state_get_settings(void)
{
    laser_ui_settings_t s = {
        .material_index = 0,
        .laser_power_pct = 50,
        .speed_pct = 100,
    };
    if (g_material_dd != nullptr) {
        s.material_index = static_cast<int>(lv_dropdown_get_selected(g_material_dd));
    }
    if (g_power_slider != nullptr) {
        s.laser_power_pct = static_cast<int>(lv_slider_get_value(g_power_slider));
    }
    if (g_speed_slider != nullptr) {
        s.speed_pct = static_cast<int>(lv_slider_get_value(g_speed_slider));
    }
    return s;
}
