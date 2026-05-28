#include "laser_ui_layout.h"

#include <lvgl.h>

void laser_ui_apply_main_gradient(lv_obj_t *obj)
{
    if (obj == nullptr) {
        return;
    }
    lv_obj_set_style_bg_color(obj, UI_COLOR_BG_BLACK, LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_NONE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
}
