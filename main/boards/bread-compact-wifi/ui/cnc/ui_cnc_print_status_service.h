#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

void ui_cnc_print_status_service_init(void);

void ui_cnc_print_status_service_on_page_show(lv_obj_t *badge, lv_obj_t *elapsed, lv_obj_t *eta,
                                              lv_obj_t *bar, lv_obj_t *pct, lv_obj_t *pos_x,
                                              lv_obj_t *pos_y);

void ui_cnc_print_status_service_on_page_hide(void);

#ifdef __cplusplus
}
#endif
