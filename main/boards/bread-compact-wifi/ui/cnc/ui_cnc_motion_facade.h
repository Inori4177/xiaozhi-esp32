#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

void ui_cnc_motion_facade_init(void);

bool ui_cnc_motion_facade_is_ready(void);

void ui_cnc_motion_facade_get_position_mm(float *x_mm, float *y_mm);

/** 插值后的显示坐标（选定页圆点跟随时使用，含运动段内实时位置）。 */
void ui_cnc_motion_facade_get_display_position_mm(float *x_mm, float *y_mm);

void ui_cnc_motion_facade_set_position_mm(float x_mm, float y_mm);

/** 运动段开始/结束：供 UI 在步进执行期间做位置插值显示。 */
void ui_cnc_motion_facade_begin_segment_mm(float from_x, float from_y, float to_x, float to_y,
                                           float feed_mm_min);

void ui_cnc_motion_facade_end_segment_mm(float x_mm, float y_mm);

/** 中止当前运动段并同步逻辑坐标（暂停/AbortMotion 后供 UI 显示）。 */
void ui_cnc_motion_facade_abort_segment_mm(void);

/** Blocking rapid move in caller task; updates logical position. */
bool ui_cnc_motion_facade_rapid_to_mm(float x_mm, float y_mm);

/** Run rapid move on dedicated task (for LVGL / confirm path). */
void ui_cnc_motion_facade_rapid_to_mm_async(float x_mm, float y_mm);

bool ui_cnc_motion_facade_is_moving(void);

#ifdef __cplusplus
}
#endif
