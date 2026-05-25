#include "page_print.h"
#include "../laser_ui_layout.h"
#include "../laser_ui_widgets.h"
#include "../laser_ui_events.h"

static void emit_cb(lv_event_t *e)
{
    auto id = static_cast<laser_ui_event_id_t>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    laser_ui_events_emit(id);
}

static void style_status_label(lv_obj_t *label, lv_color_t color)
{
    lv_obj_set_style_text_color(label, color, LV_PART_MAIN);
}

static lv_obj_t *make_jog_btn(lv_obj_t *parent, const char *text, laser_ui_event_id_t id)
{
    lv_obj_t *btn = laser_ui_create_button(parent, text, UI_COLOR_PANEL, lv_color_hex(0x2A2A2A));
    lv_obj_set_size(btn, 48, 40);
    lv_obj_add_event_cb(btn, emit_cb, LV_EVENT_CLICKED, reinterpret_cast<void *>(static_cast<intptr_t>(id)));
    return btn;
}

lv_obj_t *page_print_create(lv_obj_t *parent)
{
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_set_size(page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(page, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(page, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(page, 4, LV_PART_MAIN);
    lv_obj_set_flex_flow(page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(page, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(page, 4, LV_PART_MAIN);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *status = lv_obj_create(page);
    lv_obj_set_size(status, LV_PCT(100), 78);
    laser_ui_apply_panel_style(status, UI_COLOR_STATUS_BG, 6);
    lv_obj_set_style_border_color(status, UI_COLOR_STATUS_BORDER, LV_PART_MAIN);
    lv_obj_set_style_border_width(status, 1, LV_PART_MAIN);
    lv_obj_set_flex_flow(status, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(status, 2, LV_PART_MAIN);

    lv_obj_t *row1 = lv_obj_create(status);
    lv_obj_set_size(row1, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row1, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row1, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row1, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(row1, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row1, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *badge = lv_label_create(row1);
    lv_label_set_text(badge, " IDLE ");
    lv_obj_set_style_bg_color(badge, UI_COLOR_STATUS_IDLE_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(badge, UI_COLOR_STATUS_IDLE_FG, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(badge, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(badge, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(badge, 4, LV_PART_MAIN);

    lv_obj_t *eta = lv_label_create(row1);
    lv_label_set_text(eta, "00:00:00");
    style_status_label(eta, UI_COLOR_STATUS_META);

    lv_obj_t *bar = lv_bar_create(status);
    lv_obj_set_width(bar, LV_PCT(100));
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x2A3540), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, UI_COLOR_STATUS_VALUE, LV_PART_INDICATOR);

    lv_obj_t *row3 = lv_obj_create(status);
    lv_obj_set_size(row3, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row3, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row3, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row3, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(row3, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row3, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *pct = lv_label_create(row3);
    lv_label_set_text(pct, "0%");
    style_status_label(pct, UI_COLOR_STATUS_VALUE);
    lv_obj_t *pos = lv_label_create(row3);
    lv_label_set_text(pos, "X:0.0  Y:0.0");
    style_status_label(pos, UI_COLOR_STATUS_TITLE);

    lv_obj_t *jog = lv_obj_create(page);
    lv_obj_set_size(jog, LV_PCT(100), 118);
    laser_ui_apply_panel_style(jog, UI_COLOR_PANEL, 4);
    lv_obj_set_flex_flow(jog, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(jog, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *step_row = lv_obj_create(jog);
    lv_obj_set_size(step_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(step_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(step_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(step_row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(step_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(step_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *step_lbl = lv_label_create(step_row);
    lv_label_set_text(step_lbl, "步进 1mm");
    lv_obj_t *slider = lv_slider_create(step_row);
    lv_obj_set_width(slider, 160);
    lv_slider_set_range(slider, 0, 4);
    lv_slider_set_value(slider, 2, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, emit_cb, LV_EVENT_VALUE_CHANGED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(LASER_EVT_STEP_CHANGED)));

    lv_obj_t *cross = lv_obj_create(jog);
    lv_obj_set_size(cross, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(cross, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(cross, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(cross, 0, LV_PART_MAIN);
    lv_obj_set_layout(cross, LV_LAYOUT_GRID);
    static int32_t col_dsc[] = {48, 48, 48, LV_GRID_TEMPLATE_LAST};
    static int32_t row_dsc[] = {40, 40, 40, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(cross, col_dsc, row_dsc);

    lv_obj_t *y_plus = make_jog_btn(cross, "Y+", LASER_EVT_JOG_Y_PLUS);
    lv_obj_set_grid_cell(y_plus, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_t *x_minus = make_jog_btn(cross, "X-", LASER_EVT_JOG_X_MINUS);
    lv_obj_set_grid_cell(x_minus, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_t *home = make_jog_btn(cross, "◎", LASER_EVT_JOG_HOME);
    lv_obj_set_grid_cell(home, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_t *x_plus = make_jog_btn(cross, "X+", LASER_EVT_JOG_X_PLUS);
    lv_obj_set_grid_cell(x_plus, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_t *y_minus = make_jog_btn(cross, "Y-", LASER_EVT_JOG_Y_MINUS);
    lv_obj_set_grid_cell(y_minus, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 2, 1);

    lv_obj_t *ctrl = lv_obj_create(page);
    lv_obj_set_width(ctrl, LV_PCT(100));
    lv_obj_set_height(ctrl, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(ctrl, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(ctrl, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ctrl, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(ctrl, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ctrl, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *run = laser_ui_create_button(ctrl, "运行", UI_COLOR_RUN, lv_color_hex(0x388E3C));
    lv_obj_set_size(run, 120, 44);
    lv_obj_add_event_cb(run, emit_cb, LV_EVENT_CLICKED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(LASER_EVT_RUN)));

    lv_obj_t *pause = laser_ui_create_button(ctrl, "暂停", UI_COLOR_PAUSE, lv_color_hex(0xE65100));
    lv_obj_set_size(pause, 120, 44);
    lv_obj_add_event_cb(pause, emit_cb, LV_EVENT_CLICKED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(LASER_EVT_PAUSE)));

    return page;
}
