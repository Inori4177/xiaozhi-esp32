#include "page_print.h"
#include "../laser_ui_layout.h"
#include "../laser_ui_widgets.h"
#include "../laser_ui_events.h"
#include "../assets/laser_ui_images.h"
#include "../cnc/ui_cnc_print_status_service.h"
#include "../cnc/ui_cnc_print_status_layout.h"
#include "../cnc/ui_cnc_print_status_font.h"
#include "../../laser_ui_state.h"

static lv_obj_t *g_status_badge = nullptr;
static lv_obj_t *g_status_elapsed = nullptr;
static lv_obj_t *g_status_eta = nullptr;
static lv_obj_t *g_status_bar = nullptr;
static lv_obj_t *g_status_pct = nullptr;
static lv_obj_t *g_pos_x_label = nullptr;
static lv_obj_t *g_pos_y_label = nullptr;

/** Source asset size (btn_pad_top.c, pre-scaled at convert time). */
#define JOG_PAD_W       148
#define JOG_PAD_H       144

/** Print page vertical budget within UI_MAIN_H (260 @ 320px screen). */
#define PRINT_STATUS_H  UI_PRINT_STATUS_PANEL_H
#define PRINT_STATUS_PAD 4

/** Gap between jog pad and step / ctrl columns. */
#define PRINT_SIDE_GAP      8
#define PRINT_CTRL_BTN_GAP  20
#define PRINT_CTRL_BTN_W    96
#define PRINT_CTRL_BTN_H    44

/** Coords — top-left corner only; step must stay right of this zone. */
#define POS_COL_W           72
#define POS_ROW_GAP         6
#define POS_BLOCK_PAD_B     8
#define COORD_STEP_GAP      8

/** Step block sits left of jog pad, never under coords column. */
#define STEP_SLIDER_W       18
#define STEP_BLOCK_W        74
#define STEP_BLOCK_PAD_B    8
#define PRINT_BODY_MARGIN   6

/** Left edge of step block = margin + POS_COL_W + COORD_STEP_GAP */
#define STEP_MIN_X          (PRINT_BODY_MARGIN + POS_COL_W + COORD_STEP_GAP)

#define STATUS_BAR_H    UI_PRINT_STATUS_BAR_H
#define STATUS_PCT_W    UI_PRINT_STATUS_PCT_W
#define STATUS_ELAPSED_W UI_PRINT_STATUS_ELAPSED_W
#define STATUS_ROW_META_H UI_PRINT_STATUS_ROW_META_H
#define STATUS_ROW_PROG_H UI_PRINT_STATUS_ROW_PROG_H
#define STATUS_ROW_GAP    UI_PRINT_STATUS_ROW_GAP

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

static void style_status_label(lv_obj_t *label, lv_color_t color, int row_h = 0)
{
    ui_cnc_print_status_apply_font(label);
    lv_obj_set_style_text_color(label, color, LV_PART_MAIN);
    if (row_h > 0) {
        lv_obj_set_height(label, row_h);
        lv_obj_set_style_pad_top(label, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_bottom(label, 0, LV_PART_MAIN);
        lv_obj_set_style_text_line_space(label, 0, LV_PART_MAIN);
    }
}

static void style_status_row(lv_obj_t *row, int h)
{
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, h);
    lv_obj_set_style_min_height(row, h, LV_PART_MAIN);
    lv_obj_set_style_max_height(row, h, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
}

static void layout_no_clip(lv_obj_t *obj)
{
    lv_obj_add_flag(obj, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

static void style_panel_transparent(lv_obj_t *obj)
{
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
}

static int print_font_line_h(void)
{
    return lv_font_get_line_height(ui_cnc_print_status_font());
}

/** 行高 = 字体行高 + 上下留白，避免 descender 被裁。 */
static int print_title_row_h(void)
{
    return print_font_line_h() + 8;
}

static int print_value_row_h(void)
{
    return print_font_line_h() + 10;
}

static int pos_content_height(void)
{
    return print_title_row_h() + POS_ROW_GAP + print_value_row_h() + POS_ROW_GAP +
           print_value_row_h();
}

static int pos_block_height(void)
{
    return pos_content_height() + POS_BLOCK_PAD_B;
}

static int step_val_row_h(void)
{
    return print_value_row_h();
}

static int step_title_row_h(void)
{
    return print_title_row_h();
}

static void style_print_text_row(lv_obj_t *label, lv_color_t color, int row_h)
{
    ui_cnc_print_status_apply_font(label);
    lv_obj_set_style_text_color(label, color, LV_PART_MAIN);
    lv_obj_set_height(label, row_h);
    lv_obj_set_style_pad_top(label, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(label, 5, LV_PART_MAIN);
    lv_obj_set_style_text_line_space(label, 0, LV_PART_MAIN);
}

static void create_pos_column(lv_obj_t *parent, lv_obj_t **out_x_lbl, lv_obj_t **out_y_lbl)
{
    const int block_h = pos_content_height();

    lv_obj_t *col = lv_obj_create(parent);
    lv_obj_set_size(col, POS_COL_W, block_h);
    style_panel_transparent(col);
    layout_no_clip(col);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(col, POS_ROW_GAP, LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(col);
    lv_label_set_text(title, "坐标");
    lv_obj_set_width(title, POS_COL_W);
    style_print_text_row(title, UI_COLOR_ACCENT, print_title_row_h());

    lv_obj_t *x_lbl = lv_label_create(col);
    lv_label_set_text(x_lbl, "X:0.0");
    lv_obj_set_width(x_lbl, POS_COL_W);
    style_print_text_row(x_lbl, UI_COLOR_STATUS_TITLE, print_value_row_h());
    lv_obj_set_style_bg_color(x_lbl, lv_color_hex(0x121A28), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(x_lbl, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_radius(x_lbl, 4, LV_PART_MAIN);
    lv_obj_set_style_border_width(x_lbl, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(x_lbl, UI_COLOR_BORDER, LV_PART_MAIN);

    lv_obj_t *y_lbl = lv_label_create(col);
    lv_label_set_text(y_lbl, "Y:0.0");
    lv_obj_set_width(y_lbl, POS_COL_W);
    style_print_text_row(y_lbl, UI_COLOR_STATUS_TITLE, print_value_row_h());
    lv_obj_set_style_bg_color(y_lbl, lv_color_hex(0x121A28), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(y_lbl, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_radius(y_lbl, 4, LV_PART_MAIN);
    lv_obj_set_style_border_width(y_lbl, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(y_lbl, UI_COLOR_BORDER, LV_PART_MAIN);

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

static lv_obj_t *create_step_block(lv_obj_t *parent, int x, int y, int block_h,
                                   lv_obj_t **out_val_lbl, lv_obj_t **out_slider)
{
    const int title_h = step_title_row_h();
    const int val_h = step_val_row_h();
    const int slider_top = title_h + 6;
    const int slider_h = block_h - slider_top - val_h - STEP_BLOCK_PAD_B;

    lv_obj_t *block = lv_obj_create(parent);
    lv_obj_set_pos(block, x, y);
    lv_obj_set_size(block, STEP_BLOCK_W, block_h);
    style_panel_transparent(block);
    layout_no_clip(block);
    lv_obj_set_style_pad_bottom(block, STEP_BLOCK_PAD_B, LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(block);
    lv_label_set_text(title, "步进");
    lv_obj_set_pos(title, 0, 0);
    lv_obj_set_width(title, STEP_BLOCK_W);
    style_print_text_row(title, UI_COLOR_ACCENT, title_h);

    lv_obj_t *slider = lv_slider_create(block);
    lv_obj_set_pos(slider, (STEP_BLOCK_W - STEP_SLIDER_W) / 2, slider_top);
    lv_obj_set_size(slider, STEP_SLIDER_W, slider_h > 28 ? slider_h : 28);
    lv_slider_set_range(slider, 0, 4);
    lv_slider_set_value(slider, 0, LV_ANIM_OFF);
    laser_ui_style_energy_slider(slider, UI_COLOR_ACCENT);
#ifdef LV_SLIDER_ORIENTATION_VERTICAL
    lv_slider_set_orientation(slider, LV_SLIDER_ORIENTATION_VERTICAL);
#endif
    lv_obj_add_event_cb(slider, step_slider_cb, LV_EVENT_VALUE_CHANGED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(LASER_EVT_STEP_CHANGED)));

    lv_obj_t *val = lv_label_create(block);
    lv_label_set_text(val, "1mm");
    lv_obj_set_width(val, STEP_BLOCK_W);
    lv_obj_set_height(val, val_h);
    lv_obj_align(val, LV_ALIGN_BOTTOM_MID, 0, 0);
    style_print_text_row(val, UI_COLOR_ACCENT, val_h);
    lv_obj_set_style_text_align(val, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(val, lv_color_hex(0x121A28), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(val, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_radius(val, 4, LV_PART_MAIN);
    lv_obj_set_style_border_width(val, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(val, UI_COLOR_BORDER, LV_PART_MAIN);

    if (out_val_lbl != nullptr) {
        *out_val_lbl = val;
    }
    if (out_slider != nullptr) {
        *out_slider = slider;
    }
    return block;
}

static lv_obj_t *create_ctrl_column(lv_obj_t *parent, int x, int y)
{
    const int col_h = PRINT_CTRL_BTN_H * 2 + PRINT_CTRL_BTN_GAP;

    lv_obj_t *col = lv_obj_create(parent);
    lv_obj_set_pos(col, x, y + (JOG_PAD_H - col_h) / 2);
    lv_obj_set_size(col, PRINT_CTRL_BTN_W, col_h);
    style_panel_transparent(col);
    layout_no_clip(col);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(col, PRINT_CTRL_BTN_GAP, LV_PART_MAIN);

    lv_obj_t *run = laser_ui_create_button(col, "运行", UI_COLOR_RUN, lv_color_hex(0x388E3C));
    lv_obj_set_size(run, PRINT_CTRL_BTN_W, PRINT_CTRL_BTN_H);
    lv_obj_set_ext_click_area(run, 0);
    lv_obj_set_style_border_width(run, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(run, lv_color_hex(0x86EFAC), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(run, 4, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(run, UI_COLOR_RUN, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(run, LV_OPA_30, LV_PART_MAIN);
    lv_obj_add_event_cb(run, emit_cb, LV_EVENT_CLICKED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(LASER_EVT_RUN)));

    lv_obj_t *pause = laser_ui_create_button(col, "暂停", UI_COLOR_PAUSE, lv_color_hex(0xE65100));
    lv_obj_set_size(pause, PRINT_CTRL_BTN_W, PRINT_CTRL_BTN_H);
    lv_obj_set_ext_click_area(pause, 0);
    lv_obj_set_style_border_width(pause, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(pause, lv_color_hex(0xFCD34D), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(pause, 4, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(pause, UI_COLOR_PAUSE, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(pause, LV_OPA_30, LV_PART_MAIN);
    lv_obj_add_event_cb(pause, emit_cb, LV_EVENT_CLICKED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(LASER_EVT_PAUSE)));

    return col;
}

/**
 * 布局：坐标独占左上角列；步进仅在点动左侧，x >= STEP_MIN_X。
 * 点动/步进/按键在 body 内垂直居中，不再为坐标预留 band_top（避免整体下移）。
 */
static void create_jog_layout(lv_obj_t *parent, lv_obj_t **out_pos_x, lv_obj_t **out_pos_y,
                              lv_obj_t **out_step_lbl, lv_obj_t **out_step_slider)
{
    const int margin = PRINT_BODY_MARGIN;
    const int cw = lv_obj_get_width(parent) > 0 ? lv_obj_get_width(parent) : (UI_CONTENT_W - 8);
    const int ch = lv_obj_get_height(parent) > 0 ? lv_obj_get_height(parent) : 160;

    layout_no_clip(parent);

    const int step_min_x = STEP_MIN_X;
    const int row_w = STEP_BLOCK_W + PRINT_SIDE_GAP + JOG_PAD_W + PRINT_SIDE_GAP + PRINT_CTRL_BTN_W;
    const int row_left = (cw - row_w) / 2;
    int pad_x = row_left + STEP_BLOCK_W + PRINT_SIDE_GAP;
    int step_x = row_left;
    int ctrl_x = pad_x + JOG_PAD_W + PRINT_SIDE_GAP;

    if (step_x < step_min_x) {
        const int shift = step_min_x - step_x;
        step_x = step_min_x;
        pad_x += shift;
        ctrl_x += shift;
    }

    if (ctrl_x + PRINT_CTRL_BTN_W > cw - margin) {
        const int overflow = ctrl_x + PRINT_CTRL_BTN_W - (cw - margin);
        pad_x -= overflow;
        step_x -= overflow;
        ctrl_x -= overflow;
    }
    if (step_x < step_min_x) {
        step_x = step_min_x;
        pad_x = step_x + STEP_BLOCK_W + PRINT_SIDE_GAP;
        ctrl_x = pad_x + JOG_PAD_W + PRINT_SIDE_GAP;
    }
    if (pad_x < margin) {
        pad_x = margin;
    }

    int pad_y = (ch - JOG_PAD_H) / 2;
    if (pad_y < margin) {
        pad_y = margin;
    }
    if (pad_y + JOG_PAD_H > ch - margin) {
        pad_y = ch - margin - JOG_PAD_H;
        if (pad_y < margin) {
            pad_y = margin;
        }
    }

    lv_obj_t *pad_host = lv_obj_create(parent);
    lv_obj_set_pos(pad_host, pad_x, pad_y);
    lv_obj_set_size(pad_host, JOG_PAD_W, JOG_PAD_H);
    style_panel_transparent(pad_host);
    layout_no_clip(pad_host);
    create_jog_pad(pad_host);

    create_step_block(parent, step_x, pad_y, JOG_PAD_H, out_step_lbl, out_step_slider);
    create_ctrl_column(parent, ctrl_x, pad_y);

    lv_obj_t *pos_wrap = lv_obj_create(parent);
    lv_obj_set_pos(pos_wrap, margin, pad_y);
    lv_obj_set_size(pos_wrap, POS_COL_W, pos_block_height());
    style_panel_transparent(pos_wrap);
    layout_no_clip(pos_wrap);
    lv_obj_set_style_pad_bottom(pos_wrap, POS_BLOCK_PAD_B, LV_PART_MAIN);
    create_pos_column(pos_wrap, out_pos_x, out_pos_y);
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
    lv_obj_set_style_pad_row(page, 6, LV_PART_MAIN);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *status = laser_ui_create_hud_panel(page, UI_COLOR_STATUS_BG, PRINT_STATUS_PAD);
    lv_obj_set_size(status, LV_PCT(100), PRINT_STATUS_H);
    lv_obj_set_style_border_color(status, UI_COLOR_STATUS_BORDER, LV_PART_MAIN);
    lv_obj_set_style_border_width(status, 1, LV_PART_MAIN);
    lv_obj_remove_flag(status, LV_OBJ_FLAG_SCROLLABLE);
    laser_ui_add_panel_scanline(status, UI_COLOR_ACCENT_DIM);

    lv_obj_t *row_meta = lv_obj_create(status);
    style_status_row(row_meta, STATUS_ROW_META_H);
    lv_obj_align(row_meta, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_flex_flow(row_meta, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_meta, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(row_meta, 2, LV_PART_MAIN);

    lv_obj_t *badge = lv_label_create(row_meta);
    lv_label_set_text(badge, "空闲");
    lv_obj_set_width(badge, LV_SIZE_CONTENT);
    lv_obj_set_height(badge, LV_SIZE_CONTENT);
    ui_cnc_print_status_apply_font(badge);
    lv_obj_set_style_bg_color(badge, UI_COLOR_STATUS_IDLE_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(badge, UI_COLOR_STATUS_IDLE_FG, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(badge, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_top(badge, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(badge, 4, LV_PART_MAIN);
    lv_obj_set_style_radius(badge, 4, LV_PART_MAIN);
    g_status_badge = badge;

    lv_obj_t *eta = lv_label_create(row_meta);
    lv_label_set_text(eta, "剩余 --:--");
    lv_obj_set_width(eta, STATUS_ELAPSED_W);
    lv_obj_set_style_text_align(eta, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    style_status_label(eta, UI_COLOR_STATUS_META, STATUS_ROW_META_H);
    g_status_eta = eta;

    lv_obj_t *row_prog = lv_obj_create(status);
    style_status_row(row_prog, STATUS_ROW_PROG_H);
    lv_obj_align(row_prog, LV_ALIGN_TOP_MID, 0, STATUS_ROW_META_H + STATUS_ROW_GAP);
    lv_obj_set_flex_flow(row_prog, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_prog, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row_prog, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(row_prog, 2, LV_PART_MAIN);

    lv_obj_t *elapsed = lv_label_create(row_prog);
    lv_label_set_text(elapsed, "用时 00:00");
    lv_obj_set_width(elapsed, STATUS_ELAPSED_W);
    lv_obj_set_flex_grow(elapsed, 0);
    style_status_label(elapsed, UI_COLOR_STATUS_META, STATUS_ROW_PROG_H);
    g_status_elapsed = elapsed;

    lv_obj_t *bar = lv_bar_create(row_prog);
    lv_obj_set_flex_grow(bar, 1);
    lv_obj_set_height(bar, STATUS_BAR_H);
    lv_obj_set_style_min_height(bar, STATUS_BAR_H, LV_PART_MAIN);
    lv_obj_set_style_max_height(bar, STATUS_BAR_H, LV_PART_MAIN);
    lv_obj_set_style_pad_all(bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(bar, 0, LV_PART_MAIN);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    laser_ui_style_status_bar(bar, UI_COLOR_STATUS_VALUE);
    g_status_bar = bar;

    lv_obj_t *pct = lv_label_create(row_prog);
    lv_label_set_text(pct, "0%");
    lv_obj_set_width(pct, STATUS_PCT_W);
    lv_obj_set_flex_grow(pct, 0);
    lv_obj_set_style_text_align(pct, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    style_status_label(pct, UI_COLOR_STATUS_VALUE, STATUS_ROW_PROG_H);
    g_status_pct = pct;

    lv_obj_t *body = lv_obj_create(page);
    lv_obj_set_width(body, LV_PCT(100));
    lv_obj_set_flex_grow(body, 1);
    style_panel_transparent(body);
    layout_no_clip(body);

    lv_obj_update_layout(page);

    lv_obj_t *step_lbl = nullptr;
    lv_obj_t *step_slider = nullptr;
    create_jog_layout(body, &g_pos_x_label, &g_pos_y_label, &step_lbl, &step_slider);
    laser_ui_state_bind_print(step_lbl, step_slider);

    return page;
}

void page_print_on_show(void)
{
    ui_cnc_print_status_service_on_page_show(g_status_badge, g_status_elapsed, g_status_eta,
                                             g_status_bar, g_status_pct, g_pos_x_label,
                                             g_pos_y_label);
}

void page_print_on_hide(void)
{
    ui_cnc_print_status_service_on_page_hide();
}
