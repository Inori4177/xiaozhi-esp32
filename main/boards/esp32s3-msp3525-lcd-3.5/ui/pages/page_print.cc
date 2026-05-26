#include "page_print.h"
#include "../laser_ui_layout.h"
#include "../laser_ui_widgets.h"
#include "../laser_ui_events.h"
#include "../assets/laser_ui_images.h"
#include "../../laser_ui_state.h"

/** Source asset size (btn_pad_top.c, pre-scaled at convert time). */
#define JOG_PAD_W       148
#define JOG_PAD_H       144

/** Print page vertical budget within UI_MAIN_H (260 @ 320px screen). */
#define PRINT_STATUS_H  58
#define PRINT_STATUS_PAD 3
#define PRINT_CTRL_H    36
#define PRINT_CTRL_BTN_W  108
#define PRINT_CTRL_BTN_H  32
#define PRINT_PAGE_PAD  12
#define JOG_CTRL_GAP    6

/** XY position column left of step slider. */
#define POS_COL_W       92

/** Step column left of jog pad. */
#define STEP_COL_W      56
#define STEP_SLIDER_W   16
#define STEP_TITLE_H    20
#define STEP_VAL_H      22

#define STATUS_BAR_H    10
#define STATUS_PCT_W    40

/** Jog row height matches pad asset height. */
#define JOG_ROW_H       JOG_PAD_H

#define JOG_ARROW_SZ    30
#define JOG_HIT_SZ      42
#define JOG_CENTER_SZ   46
#define JOG_EDGE_OFS    6

static void emit_cb(lv_event_t *e)
{
    auto id = static_cast<laser_ui_event_id_t>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    laser_ui_events_emit(id);
}

static void step_slider_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    laser_ui_state_on_step_slider(e);
}

static void style_status_label(lv_obj_t *label, lv_color_t color)
{
    lv_obj_set_style_text_color(label, color, LV_PART_MAIN);
}

static void style_transparent_row(lv_obj_t *row)
{
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
}

static void create_pos_column(lv_obj_t *parent, int col_h, lv_obj_t **out_x_lbl, lv_obj_t **out_y_lbl)
{
    lv_obj_t *col = lv_obj_create(parent);
    lv_obj_set_size(col, POS_COL_W, col_h);
    lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(col, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(col, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(col, 4, LV_PART_MAIN);
    lv_obj_remove_flag(col, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(col);
    lv_label_set_text(title, "坐标");
    lv_obj_set_width(title, POS_COL_W);
    lv_obj_set_style_text_color(title, UI_COLOR_ACCENT, LV_PART_MAIN);

    lv_obj_t *x_lbl = lv_label_create(col);
    lv_label_set_text(x_lbl, "X:0.0");
    lv_obj_set_width(x_lbl, POS_COL_W);
    style_status_label(x_lbl, UI_COLOR_STATUS_TITLE);

    lv_obj_t *y_lbl = lv_label_create(col);
    lv_label_set_text(y_lbl, "Y:0.0");
    lv_obj_set_width(y_lbl, POS_COL_W);
    style_status_label(y_lbl, UI_COLOR_STATUS_TITLE);

    if (out_x_lbl != nullptr) {
        *out_x_lbl = x_lbl;
    }
    if (out_y_lbl != nullptr) {
        *out_y_lbl = y_lbl;
    }
}

static void style_pad_hit_button(lv_obj_t *btn)
{
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, LV_OPA_20, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(btn, UI_COLOR_ACCENT, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, LV_PART_MAIN);
}

static lv_obj_t *make_pad_arrow_btn(lv_obj_t *pad, const lv_image_dsc_t *img_dsc,
                                    laser_ui_event_id_t id, lv_align_t align, int x_ofs, int y_ofs)
{
    lv_obj_t *btn = lv_button_create(pad);
    lv_obj_set_size(btn, JOG_HIT_SZ, JOG_HIT_SZ);
    style_pad_hit_button(btn);
    lv_obj_align(btn, align, x_ofs, y_ofs);
    lv_obj_add_event_cb(btn, emit_cb, LV_EVENT_CLICKED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(id)));

    lv_obj_t *img = lv_image_create(btn);
    lv_image_set_src(img, img_dsc);
    lv_obj_set_size(img, JOG_ARROW_SZ, JOG_ARROW_SZ);
    lv_obj_center(img);
    lv_obj_remove_flag(img, LV_OBJ_FLAG_CLICKABLE);
    return btn;
}

static lv_obj_t *create_jog_pad(lv_obj_t *parent)
{
    lv_obj_t *pad = lv_obj_create(parent);
    lv_obj_set_size(pad, JOG_PAD_W, JOG_PAD_H);
    lv_obj_set_style_bg_opa(pad, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(pad, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(pad, 0, LV_PART_MAIN);
    lv_obj_remove_flag(pad, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *bg = lv_image_create(pad);
    lv_image_set_src(bg, &btn_pad_top);
    lv_obj_align(bg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_remove_flag(bg, LV_OBJ_FLAG_CLICKABLE);

    make_pad_arrow_btn(pad, &arrow_up, LASER_EVT_JOG_Y_PLUS, LV_ALIGN_TOP_MID, 0, JOG_EDGE_OFS);
    make_pad_arrow_btn(pad, &arrow_down, LASER_EVT_JOG_Y_MINUS, LV_ALIGN_BOTTOM_MID, 0, -JOG_EDGE_OFS);
    make_pad_arrow_btn(pad, &arrow_left, LASER_EVT_JOG_X_MINUS, LV_ALIGN_LEFT_MID, JOG_EDGE_OFS, 0);
    make_pad_arrow_btn(pad, &arrow_right, LASER_EVT_JOG_X_PLUS, LV_ALIGN_RIGHT_MID, -JOG_EDGE_OFS, 0);

    /* Labels on pad (not inside home) so "MOVE" is not clipped by the 46px hit target. */
    lv_obj_t *move_lbl = lv_label_create(pad);
    lv_label_set_text(move_lbl, "MOVE");
    lv_obj_set_style_text_color(move_lbl, UI_COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_text_align(move_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(move_lbl, LV_ALIGN_CENTER, 0, -6);
    lv_obj_remove_flag(move_lbl, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *xy_lbl = lv_label_create(pad);
    lv_label_set_text(xy_lbl, "XY");
    lv_obj_set_style_text_color(xy_lbl, UI_COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_text_align(xy_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(xy_lbl, LV_ALIGN_CENTER, 0, 8);
    lv_obj_remove_flag(xy_lbl, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *home = lv_button_create(pad);
    lv_obj_set_size(home, JOG_CENTER_SZ, JOG_CENTER_SZ);
    style_pad_hit_button(home);
    lv_obj_align(home, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(home, emit_cb, LV_EVENT_CLICKED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(LASER_EVT_JOG_HOME)));

    return pad;
}

static void create_step_column(lv_obj_t *parent, int col_h, lv_obj_t **out_val_lbl, lv_obj_t **out_slider)
{
    lv_obj_t *col = lv_obj_create(parent);
    lv_obj_set_size(col, STEP_COL_W, col_h);
    lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(col, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(col, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(col, 2, LV_PART_MAIN);
    lv_obj_remove_flag(col, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(col);
    lv_label_set_text(title, "步进");
    lv_obj_set_width(title, STEP_COL_W);
    lv_obj_set_height(title, STEP_TITLE_H);
    lv_obj_set_style_text_color(title, UI_COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    lv_obj_t *slider = lv_slider_create(col);
    lv_obj_set_width(slider, STEP_SLIDER_W);
    lv_obj_set_flex_grow(slider, 1);
    lv_slider_set_range(slider, 0, 4);
    lv_slider_set_value(slider, 0, LV_ANIM_OFF);
#ifdef LV_SLIDER_ORIENTATION_VERTICAL
    lv_slider_set_orientation(slider, LV_SLIDER_ORIENTATION_VERTICAL);
#endif
    lv_obj_add_event_cb(slider, step_slider_cb, LV_EVENT_VALUE_CHANGED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(LASER_EVT_STEP_CHANGED)));

    lv_obj_t *val = lv_label_create(col);
    lv_label_set_text(val, "1mm");
    lv_obj_set_width(val, STEP_COL_W);
    lv_obj_set_height(val, STEP_VAL_H);
    lv_obj_set_style_text_color(val, UI_COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_text_align(val, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    if (out_val_lbl != nullptr) {
        *out_val_lbl = val;
    }
    if (out_slider != nullptr) {
        *out_slider = slider;
    }
}

static lv_obj_t *create_jog_row(lv_obj_t *parent, lv_obj_t **out_pos_x, lv_obj_t **out_pos_y,
                                lv_obj_t **out_step_lbl, lv_obj_t **out_step_slider)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), JOG_ROW_H);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(row, 6, LV_PART_MAIN);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    create_pos_column(row, JOG_ROW_H, out_pos_x, out_pos_y);
    create_step_column(row, JOG_ROW_H, out_step_lbl, out_step_slider);

    lv_obj_set_style_margin_bottom(row, JOG_CTRL_GAP, LV_PART_MAIN);

    lv_obj_t *pad_host = lv_obj_create(row);
    lv_obj_set_size(pad_host, JOG_PAD_W, JOG_PAD_H);
    lv_obj_set_style_bg_opa(pad_host, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(pad_host, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(pad_host, 0, LV_PART_MAIN);
    lv_obj_remove_flag(pad_host, LV_OBJ_FLAG_SCROLLABLE);

    create_jog_pad(pad_host);
    return row;
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
    lv_obj_set_size(status, LV_PCT(100), PRINT_STATUS_H);
    laser_ui_apply_panel_style(status, UI_COLOR_STATUS_BG, PRINT_STATUS_PAD);
    lv_obj_set_style_border_color(status, UI_COLOR_STATUS_BORDER, LV_PART_MAIN);
    lv_obj_set_style_border_width(status, 1, LV_PART_MAIN);
    lv_obj_set_flex_flow(status, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(status, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(status, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(status, 2, LV_PART_MAIN);
    lv_obj_remove_flag(status, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *row_meta = lv_obj_create(status);
    style_transparent_row(row_meta);
    lv_obj_set_flex_flow(row_meta, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_meta, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *badge = lv_label_create(row_meta);
    lv_label_set_text(badge, " IDLE ");
    lv_obj_set_style_bg_color(badge, UI_COLOR_STATUS_IDLE_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(badge, UI_COLOR_STATUS_IDLE_FG, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(badge, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(badge, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(badge, 4, LV_PART_MAIN);

    lv_obj_t *eta = lv_label_create(row_meta);
    lv_label_set_text(eta, "剩余 00:00:00");
    style_status_label(eta, UI_COLOR_STATUS_META);

    lv_obj_t *row_prog = lv_obj_create(status);
    style_transparent_row(row_prog);
    lv_obj_set_flex_flow(row_prog, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_prog, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row_prog, 6, LV_PART_MAIN);
    lv_obj_set_style_margin_top(row_prog, -1, LV_PART_MAIN);

    lv_obj_t *bar = lv_bar_create(row_prog);
    lv_obj_set_flex_grow(bar, 1);
    lv_obj_set_height(bar, STATUS_BAR_H);
    lv_obj_set_style_pad_ver(bar, 0, LV_PART_MAIN);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x2A3540), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, UI_COLOR_STATUS_VALUE, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 3, LV_PART_INDICATOR);

    lv_obj_t *pct = lv_label_create(row_prog);
    lv_label_set_text(pct, "0%");
    lv_obj_set_width(pct, STATUS_PCT_W);
    lv_obj_set_height(pct, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(pct, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(pct, 0, LV_PART_MAIN);
    style_status_label(pct, UI_COLOR_STATUS_VALUE);

    lv_obj_t *step_lbl = nullptr;
    lv_obj_t *step_slider = nullptr;
    create_jog_row(page, nullptr, nullptr, &step_lbl, &step_slider);
    laser_ui_state_bind_print(step_lbl, step_slider);

    lv_obj_t *ctrl = lv_obj_create(page);
    lv_obj_set_width(ctrl, LV_PCT(100));
    lv_obj_set_height(ctrl, PRINT_CTRL_H);
    lv_obj_set_style_bg_opa(ctrl, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(ctrl, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ctrl, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(ctrl, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ctrl, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *run = laser_ui_create_button(ctrl, "运行", UI_COLOR_RUN, lv_color_hex(0x388E3C));
    lv_obj_set_size(run, PRINT_CTRL_BTN_W, PRINT_CTRL_BTN_H);
    lv_obj_add_event_cb(run, emit_cb, LV_EVENT_CLICKED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(LASER_EVT_RUN)));

    lv_obj_t *pause = laser_ui_create_button(ctrl, "暂停", UI_COLOR_PAUSE, lv_color_hex(0xE65100));
    lv_obj_set_size(pause, PRINT_CTRL_BTN_W, PRINT_CTRL_BTN_H);
    lv_obj_add_event_cb(pause, emit_cb, LV_EVENT_CLICKED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(LASER_EVT_PAUSE)));

    return page;
}
