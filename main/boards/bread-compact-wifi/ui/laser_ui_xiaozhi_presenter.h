#pragma once

#include <lvgl.h>

#include "laser_ui_shell.h"

void laser_ui_xiaozhi_presenter_init(lv_obj_t *screen);
void laser_ui_xiaozhi_presenter_bind_voice_page(lv_obj_t *page);
void laser_ui_xiaozhi_presenter_set_current_page(LaserPage page);
void laser_ui_xiaozhi_presenter_raise_overlay(void);
