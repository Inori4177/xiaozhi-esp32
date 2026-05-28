#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

void ui_pick_service_init(void);

/** Called from page_pick when view becomes visible. */
void ui_pick_service_on_page_show(lv_obj_t *work_area, lv_obj_t *head_dot, lv_obj_t *cursor_dot,
                                  lv_obj_t *coord_label, lv_obj_t *status_label);

void ui_pick_service_on_page_hide(void);

/** Cursor mm for confirm (updated while dragging). */
void ui_pick_service_get_cursor_mm(float *x_mm, float *y_mm);

void ui_pick_service_set_cursor_mm(float x_mm, float y_mm);

/** Local pixel coords inside work_area → cursor mm (preview only). */
void ui_pick_service_set_cursor_from_local_px(int local_x, int local_y);

void ui_pick_service_update_viewport_from_work_area(void);

#ifdef __cplusplus
}
#endif
