#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int material_index;
    int laser_power_pct;
    /** Motion speed as percent; 100 means baseline feed (LASER_GCODE_FEED_BASE_MM_MIN). */
    int speed_pct;
} laser_ui_settings_t;

void laser_ui_state_init(void);

void laser_ui_state_bind_print(lv_obj_t *step_label, lv_obj_t *step_slider);
void laser_ui_state_bind_settings(lv_obj_t *material_dd, lv_obj_t *power_slider,
                                  lv_obj_t *power_label, lv_obj_t *speed_slider,
                                  lv_obj_t *speed_label);

void laser_ui_state_on_step_slider(lv_event_t *e);
void laser_ui_state_on_power_slider(lv_event_t *e);
void laser_ui_state_on_speed_slider(lv_event_t *e);
void laser_ui_state_on_material_changed(lv_event_t *e);

float laser_ui_state_get_jog_step_mm(void);
laser_ui_settings_t laser_ui_state_get_settings(void);

/** WebUI / 无 LVGL 控件时直接写入缓存（与屏幕档位一致）。 */
void laser_ui_state_set_jog_step_mm(float step_mm);
void laser_ui_state_set_power_pct(int pct);
void laser_ui_state_set_speed_pct(int pct);

void laser_ui_state_set_pick_origin(float x_mm, float y_mm);
bool laser_ui_state_get_pick_origin(float *x_mm, float *y_mm);
void laser_ui_state_clear_pick_origin(void);

#ifdef __cplusplus
}
#endif
