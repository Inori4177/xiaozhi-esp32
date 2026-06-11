#pragma once

#include "../cnc/ui_cnc_coord_map.h"

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

void ui_pick_service_init(void);

/** @param map_frame Square 42×42 mm mapping area (child widgets use its local coords). */
void ui_pick_service_on_page_show(lv_obj_t *map_frame, lv_obj_t *head_dot, lv_obj_t *cursor_cross,
                                  lv_obj_t *coord_x_label, lv_obj_t *coord_y_label,
                                  lv_obj_t *status_label);

void ui_pick_service_on_page_hide(void);

/** Cursor mm for confirm (updated while dragging). */
void ui_pick_service_get_cursor_mm(float *x_mm, float *y_mm);

void ui_pick_service_set_cursor_mm(float x_mm, float y_mm);

/** Touch down — reset filter and jump to finger. */
void ui_pick_service_touch_begin(int local_x, int local_y);

/** Drag — filtered; ignores small jitter. */
void ui_pick_service_set_cursor_from_local_px(int local_x, int local_y);

/** Touch up — lock last stable drag position; snap to 1 mm grid. */
void ui_pick_service_touch_end(int local_x, int local_y);

void ui_pick_service_update_viewport_from_map_frame(void);

const ui_cnc_coord_viewport_t *ui_pick_service_get_viewport(void);

#ifdef __cplusplus
}
#endif
