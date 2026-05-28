#pragma once

#include <lvgl.h>

lv_obj_t *laser_ui_apply_panel_style(lv_obj_t *obj, lv_color_t bg, int pad);
void laser_ui_apply_button_style(lv_obj_t *btn, lv_color_t bg, lv_color_t pressed_bg);
lv_obj_t *laser_ui_create_button(lv_obj_t *parent, const char *text, lv_color_t bg, lv_color_t pressed_bg);
lv_obj_t *laser_ui_create_nav_dock_button(lv_obj_t *parent, const char *icon);
lv_obj_t *laser_ui_create_tab_button(lv_obj_t *parent, const char *text, bool active);
