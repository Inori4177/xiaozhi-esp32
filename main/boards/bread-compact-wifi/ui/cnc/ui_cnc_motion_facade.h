#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

void ui_cnc_motion_facade_init(void);

bool ui_cnc_motion_facade_is_ready(void);

void ui_cnc_motion_facade_get_position_mm(float *x_mm, float *y_mm);

void ui_cnc_motion_facade_set_position_mm(float x_mm, float y_mm);

/** Blocking rapid move in caller task; updates logical position. */
bool ui_cnc_motion_facade_rapid_to_mm(float x_mm, float y_mm);

/** Run rapid move on dedicated task (for LVGL / confirm path). */
void ui_cnc_motion_facade_rapid_to_mm_async(float x_mm, float y_mm);

bool ui_cnc_motion_facade_is_moving(void);

#ifdef __cplusplus
}
#endif
