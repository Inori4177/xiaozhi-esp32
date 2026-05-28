#include "laser_ui_layout.h"

#include <lvgl.h>

#if LV_USE_DRAW_SW_COMPLEX_GRADIENTS

static lv_style_t s_main_bg_style;
static lv_grad_dsc_t s_main_bg_grad;
static bool s_main_bg_ready = false;

#endif

static void apply_simple_soft_gradient(lv_obj_t *obj)
{
    /* Fallback: 2-stop vertical blend when complex gradients are off. */
    lv_obj_set_style_bg_color(obj, UI_COLOR_BG_BLACK, LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0x1A3070), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, LV_PART_MAIN);
    lv_obj_set_style_bg_main_stop(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_grad_stop(obj, 200, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
}

void laser_ui_apply_main_gradient(lv_obj_t *obj)
{
    if (obj == nullptr) {
        return;
    }

#if LV_USE_DRAW_SW_COMPLEX_GRADIENTS
    if (!s_main_bg_ready) {
        /* 6 stops: ~70% black, soft foggy transition, ~30% blue (TL→BR). */
        static const lv_color_t colors[] = {
            UI_COLOR_BG_BLACK,           /* 0   pure black */
            lv_color_hex(0x03030A),      /* 96  near black */
            lv_color_hex(0x0A1430),      /* 145 dark navy haze */
            lv_color_hex(0x142858),      /* 178 ~70% boundary */
            lv_color_hex(0x1E40A0),      /* 210 mid blue */
            UI_COLOR_BG_BLUE,            /* 255 saturated blue */
        };
        static const uint8_t fracs[] = {0, 96, 145, 178, 210, 255};

        lv_style_init(&s_main_bg_style);
        lv_grad_init_stops(&s_main_bg_grad, colors, nullptr, fracs, 6);
        lv_grad_linear_init(&s_main_bg_grad, lv_pct(0), lv_pct(0), lv_pct(100), lv_pct(100), LV_GRAD_EXTEND_PAD);
        lv_style_set_bg_grad(&s_main_bg_style, &s_main_bg_grad);
        lv_style_set_bg_opa(&s_main_bg_style, LV_OPA_COVER);
        s_main_bg_ready = true;
    }
    lv_obj_add_style(obj, &s_main_bg_style, LV_PART_MAIN);
#else
    apply_simple_soft_gradient(obj);
#endif
}
