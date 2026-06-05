#include "laser_ui_layout.h"

#include <lvgl.h>

/**
 * 主界面渐变背景 — 使用纯深色 + 微妙的垂直渐变
 * 从极深色过渡到稍亮的深蓝色，营造沉浸感
 */
void laser_ui_apply_main_gradient(lv_obj_t *obj)
{
    if (obj == nullptr) {
        return;
    }
    /* 纯色深底，避免渐变造成性能开销；如需渐变可改用 LV_GRAD_DIR_VER */
    lv_obj_set_style_bg_color(obj, UI_COLOR_PAGE_BG_TOP, LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(obj, UI_COLOR_PAGE_BG_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
}
