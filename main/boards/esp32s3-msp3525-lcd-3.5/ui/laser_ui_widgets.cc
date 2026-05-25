#include "laser_ui_widgets.h"
#include "laser_ui_layout.h"

lv_obj_t *laser_ui_apply_panel_style(lv_obj_t *obj, lv_color_t bg, int pad)
{
    lv_obj_set_style_radius(obj, 6, LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj, bg, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(obj, UI_COLOR_BORDER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, pad, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN);
    return obj;
}

void laser_ui_apply_button_style(lv_obj_t *btn, lv_color_t bg, lv_color_t pressed_bg)
{
    lv_obj_set_style_radius(btn, 6, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, pressed_bg, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_text_color(btn, UI_COLOR_TEXT, LV_PART_MAIN);
    lv_obj_set_style_transform_width(btn, -2, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_transform_height(btn, -2, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_ext_click_area(btn, 8);
}

lv_obj_t *laser_ui_create_button(lv_obj_t *parent, const char *text, lv_color_t bg, lv_color_t pressed_bg)
{
    lv_obj_t *btn = lv_button_create(parent);
    laser_ui_apply_button_style(btn, bg, pressed_bg);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    return btn;
}

lv_obj_t *laser_ui_create_nav_button(lv_obj_t *parent, const char *icon, const char *label_text)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, UI_NAV_W - 8, 58);
    lv_obj_set_style_radius(btn, 4, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, UI_COLOR_NAV, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, UI_COLOR_NAV_ACTIVE, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x252525), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 3, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_border_color(btn, CYBER_CYAN, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(btn, 2, LV_PART_MAIN);
    lv_obj_set_style_transform_width(btn, -1, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_transform_height(btn, -1, LV_PART_MAIN | LV_STATE_PRESSED);

    lv_obj_t *icon_lbl = lv_label_create(btn);
    lv_label_set_text(icon_lbl, icon);
    lv_obj_set_style_text_color(icon_lbl, UI_COLOR_TEXT, LV_PART_MAIN);

    lv_obj_t *text_lbl = lv_label_create(btn);
    lv_label_set_text(text_lbl, label_text);
    lv_obj_set_style_text_color(text_lbl, UI_COLOR_TEXT_DIM, LV_PART_MAIN);

    return btn;
}

lv_obj_t *laser_ui_create_tab_button(lv_obj_t *parent, const char *text, bool active)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_height(btn, 28);
    lv_obj_set_style_pad_hor(btn, 10, LV_PART_MAIN);
    lv_color_t bg = active ? UI_COLOR_NAV_ACTIVE : UI_COLOR_PANEL;
    lv_color_t pressed = active ? lv_color_hex(0x244A31) : lv_color_hex(0x2A2A2A);
    laser_ui_apply_button_style(btn, bg, pressed);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_center(lbl);
    return btn;
}
