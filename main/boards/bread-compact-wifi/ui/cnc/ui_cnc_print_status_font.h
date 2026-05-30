#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 状态栏标签字体（复用 BUILTIN_TEXT_FONT，不额外链接 font_puhui_14_1 以节省 app 体积）。 */
const lv_font_t *ui_cnc_print_status_font(void);

void ui_cnc_print_status_apply_font(lv_obj_t *obj);

#ifdef __cplusplus
}
#endif
