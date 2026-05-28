#include "page_xiaozhi.h"
#include "../laser_ui_layout.h"
#include "../laser_ui_widgets.h"
#include "../laser_ui_log.h"

static lv_obj_t *g_log_label = nullptr;

void page_xiaozhi_refresh_log(void)
{
    if (g_log_label == nullptr) {
        return;
    }
    const char *text = laser_ui_log_get_text();
    lv_label_set_text(g_log_label, (text != nullptr && text[0] != '\0') ? text : "暂无对话记录");
    lv_obj_scroll_to_y(lv_obj_get_parent(g_log_label), LV_COORD_MAX, LV_ANIM_OFF);
}

lv_obj_t *page_xiaozhi_create(lv_obj_t *parent)
{
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_set_size(page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(page, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(page, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(page, 4, LV_PART_MAIN);
    lv_obj_set_flex_flow(page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(page, 4, LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(page);
    lv_label_set_text(title, "对话记录");
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT, LV_PART_MAIN);

    lv_obj_t *scroll = lv_obj_create(page);
    lv_obj_set_width(scroll, LV_PCT(100));
    lv_obj_set_flex_grow(scroll, 1);
    laser_ui_apply_panel_style(scroll, UI_COLOR_PANEL, 6);
    lv_obj_set_scrollbar_mode(scroll, LV_SCROLLBAR_MODE_AUTO);

    g_log_label = lv_label_create(scroll);
    lv_obj_set_width(g_log_label, LV_PCT(100));
    lv_label_set_long_mode(g_log_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(g_log_label, UI_COLOR_TEXT_DIM, LV_PART_MAIN);
    page_xiaozhi_refresh_log();

    return page;
}
