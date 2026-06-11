#include "page_print.h"
#include "../laser_ui_layout.h"
#include "../laser_ui_widgets.h"
#include "../laser_ui_events.h"
#include "../assets/laser_ui_images.h"
#include "../cnc/ui_cnc_print_status_service.h"
#include "../cnc/ui_cnc_print_status_layout.h"
#include "../cnc/ui_cnc_print_status_font.h"
#include "../../laser_ui_state.h"

#include <cstdint>

#include <esp_log.h>

static const char *TAG = "page_print";

static lv_obj_t *g_status_badge = nullptr;
static lv_obj_t *g_status_elapsed = nullptr;
static lv_obj_t *g_status_eta = nullptr;
static lv_obj_t *g_status_bar = nullptr;
static lv_obj_t *g_status_pct = nullptr;
static lv_obj_t *g_pos_x_label = nullptr;
static lv_obj_t *g_pos_y_label = nullptr;

#define PRINT_STATUS_H       UI_PRINT_STATUS_PANEL_H
#define PRINT_STATUS_PAD     4
#define STATUS_BAR_H         UI_PRINT_STATUS_BAR_H
#define STATUS_PCT_W         UI_PRINT_STATUS_PCT_W
#define STATUS_ELAPSED_W     UI_PRINT_STATUS_ELAPSED_W
#define STATUS_ROW_META_H    UI_PRINT_STATUS_ROW_META_H
#define STATUS_ROW_PROG_H    UI_PRINT_STATUS_ROW_PROG_H
#define STATUS_ROW_GAP       UI_PRINT_STATUS_ROW_GAP

#define PRINT_STATUS_GAP     6
#define PRINT_BODY_Y         (PRINT_STATUS_H + PRINT_STATUS_GAP)
#define PRINT_BODY_H         (UI_MAIN_H - PRINT_BODY_Y)
#define PRINT_BG_W           426
#define PRINT_BG_H           187
#define PRINT_BG_X           0
#define PRINT_BG_Y           ((PRINT_BODY_H - PRINT_BG_H) / 2)

/* Coordinates are relative to print.png's top-left corner. */
#define STEP_TITLE_X         32
#define STEP_TITLE_Y         30
#define STEP_TITLE_W         72
#define STEP_TITLE_H         28
#define STEP_SLIDER_X        29
#define STEP_SLIDER_Y        72
#define STEP_SLIDER_W        86
#define STEP_SLIDER_H        18
#define STEP_VALUE_X         70
#define STEP_VALUE_Y         92
#define STEP_VALUE_W         45
#define STEP_VALUE_H         24

#define RUN_HIT_X            318
#define RUN_HIT_Y            42
#define RUN_HIT_W            88
#define RUN_HIT_H            54
#define PAUSE_HIT_X          318
#define PAUSE_HIT_Y          102
#define PAUSE_HIT_W          88
#define PAUSE_HIT_H          55

#define JOG_UP_X             186
#define JOG_UP_Y             24
#define JOG_UP_W             62
#define JOG_UP_H             50
#define JOG_LEFT_X           144
#define JOG_LEFT_Y           55
#define JOG_LEFT_W           56
#define JOG_LEFT_H           70
#define JOG_HOME_X           188
#define JOG_HOME_Y           72
#define JOG_HOME_W           50
#define JOG_HOME_H           50
#define JOG_RIGHT_X          236
#define JOG_RIGHT_Y          55
#define JOG_RIGHT_W          56
#define JOG_RIGHT_H          70
#define JOG_DOWN_X           186
#define JOG_DOWN_Y           119
#define JOG_DOWN_W           62
#define JOG_DOWN_H           50

#define POS_X_LABEL_X        22
#define POS_X_LABEL_Y        124
#define POS_Y_LABEL_X        22
#define POS_Y_LABEL_Y        151
#define POS_LABEL_W          98
#define POS_LABEL_H          24

static void emit_cb(lv_event_t *e)
{
    auto id = static_cast<laser_ui_event_id_t>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    ESP_LOGI(TAG, "touch -> %s", laser_ui_event_name(id));
    laser_ui_events_emit(id);
}

static void step_slider_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    ESP_LOGI(TAG, "step slider changed");
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

static void style_transparent_panel(lv_obj_t *obj)
{
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

static void style_overlay_button(lv_obj_t *btn, lv_color_t press_color, int radius)
{
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, press_color, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_30, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, radius, LV_PART_MAIN);
    lv_obj_set_ext_click_area(btn, 0);
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
}

static lv_obj_t *create_hot_button(lv_obj_t *parent, int x, int y, int w, int h,
                                   laser_ui_event_id_t id, lv_color_t press_color,
                                   int radius)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_pos(btn, PRINT_BG_X + x, PRINT_BG_Y + y);
    lv_obj_set_size(btn, w, h);
    style_overlay_button(btn, press_color, radius);
    lv_obj_add_event_cb(btn, emit_cb, LV_EVENT_CLICKED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(id)));
    return btn;
}

static lv_obj_t *create_text_label(lv_obj_t *parent, const char *text,
                                   int x, int y, int w, int h,
                                   lv_color_t color, lv_text_align_t align)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, PRINT_BG_X + x, PRINT_BG_Y + y);
    lv_obj_set_size(label, w, h);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(label, color, LV_PART_MAIN);
    lv_obj_set_style_text_align(label, align, LV_PART_MAIN);
    lv_obj_set_style_pad_all(label, 0, LV_PART_MAIN);
    lv_obj_remove_flag(label, LV_OBJ_FLAG_CLICKABLE);
    return label;
}

static void create_print_body(lv_obj_t *body)
{
    style_transparent_panel(body);

    lv_obj_t *bg = lv_image_create(body);
    lv_image_set_src(bg, &print);
    lv_obj_set_pos(bg, PRINT_BG_X, PRINT_BG_Y);
    lv_obj_remove_flag(bg, LV_OBJ_FLAG_CLICKABLE);

    create_text_label(body, "步进", STEP_TITLE_X, STEP_TITLE_Y, STEP_TITLE_W, STEP_TITLE_H,
                      UI_COLOR_TEXT, LV_TEXT_ALIGN_LEFT);

    lv_obj_t *step_slider = lv_slider_create(body);
    lv_obj_set_pos(step_slider, PRINT_BG_X + STEP_SLIDER_X, PRINT_BG_Y + STEP_SLIDER_Y);
    lv_obj_set_size(step_slider, STEP_SLIDER_W, STEP_SLIDER_H);
    lv_slider_set_range(step_slider, 0, 4);
    lv_slider_set_value(step_slider, 0, LV_ANIM_OFF);
    laser_ui_style_energy_slider(step_slider, UI_COLOR_ACCENT);
    lv_obj_add_event_cb(step_slider, step_slider_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t *step_lbl = create_text_label(body, "1mm", STEP_VALUE_X, STEP_VALUE_Y,
                                           STEP_VALUE_W, STEP_VALUE_H,
                                           UI_COLOR_TEXT, LV_TEXT_ALIGN_CENTER);
    laser_ui_state_bind_print(step_lbl, step_slider);

    g_pos_x_label = create_text_label(body, "X:0.0mm", POS_X_LABEL_X, POS_X_LABEL_Y,
                                      POS_LABEL_W, POS_LABEL_H, UI_COLOR_TEXT,
                                      LV_TEXT_ALIGN_LEFT);
    g_pos_y_label = create_text_label(body, "Y:0.0mm", POS_Y_LABEL_X, POS_Y_LABEL_Y,
                                      POS_LABEL_W, POS_LABEL_H, UI_COLOR_TEXT,
                                      LV_TEXT_ALIGN_LEFT);

    create_hot_button(body, RUN_HIT_X, RUN_HIT_Y, RUN_HIT_W, RUN_HIT_H,
                      LASER_EVT_RUN, UI_COLOR_RUN, 10);
    create_hot_button(body, PAUSE_HIT_X, PAUSE_HIT_Y, PAUSE_HIT_W, PAUSE_HIT_H,
                      LASER_EVT_PAUSE, UI_COLOR_PAUSE, 10);

    create_hot_button(body, JOG_UP_X, JOG_UP_Y, JOG_UP_W, JOG_UP_H,
                      LASER_EVT_JOG_Y_PLUS, UI_COLOR_ACCENT, LV_RADIUS_CIRCLE);
    create_hot_button(body, JOG_DOWN_X, JOG_DOWN_Y, JOG_DOWN_W, JOG_DOWN_H,
                      LASER_EVT_JOG_Y_MINUS, UI_COLOR_ACCENT, LV_RADIUS_CIRCLE);
    create_hot_button(body, JOG_LEFT_X, JOG_LEFT_Y, JOG_LEFT_W, JOG_LEFT_H,
                      LASER_EVT_JOG_X_MINUS, UI_COLOR_ACCENT, LV_RADIUS_CIRCLE);
    create_hot_button(body, JOG_RIGHT_X, JOG_RIGHT_Y, JOG_RIGHT_W, JOG_RIGHT_H,
                      LASER_EVT_JOG_X_PLUS, UI_COLOR_ACCENT, LV_RADIUS_CIRCLE);
    create_hot_button(body, JOG_HOME_X, JOG_HOME_Y, JOG_HOME_W, JOG_HOME_H,
                      LASER_EVT_JOG_HOME, UI_COLOR_ACCENT, LV_RADIUS_CIRCLE);
}

static void create_print_body_area(lv_obj_t *page)
{
    lv_obj_t *body = lv_obj_create(page);
    lv_obj_set_pos(body, 0, PRINT_BODY_Y);
    lv_obj_set_size(body, UI_CONTENT_W, PRINT_BODY_H);
    create_print_body(body);
}

lv_obj_t *page_print_create(lv_obj_t *parent)
{
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_set_size(page, UI_CONTENT_W, UI_MAIN_H);
    lv_obj_set_pos(page, 0, 0);
    lv_obj_set_layout(page, LV_LAYOUT_NONE);
    lv_obj_set_style_bg_opa(page, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(page, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(page, 0, LV_PART_MAIN);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *status = laser_ui_create_hud_panel(page, UI_COLOR_CARD, PRINT_STATUS_PAD);
    lv_obj_set_pos(status, 0, 0);
    lv_obj_set_size(status, UI_CONTENT_W, PRINT_STATUS_H);
    lv_obj_set_style_border_color(status, UI_COLOR_LIGHT_BORDER, LV_PART_MAIN);
    lv_obj_set_style_border_width(status, 1, LV_PART_MAIN);
    lv_obj_remove_flag(status, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *row_meta = lv_obj_create(status);
    style_status_row(row_meta, STATUS_ROW_META_H);
    lv_obj_align(row_meta, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_flex_flow(row_meta, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_meta, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(row_meta, 2, LV_PART_MAIN);

    lv_obj_t *badge = lv_label_create(row_meta);
    lv_label_set_text(badge, "空闲");
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
    style_status_label(eta, UI_COLOR_TEXT_SEC, STATUS_ROW_META_H);
    g_status_eta = eta;

    lv_obj_t *row_prog = lv_obj_create(status);
    style_status_row(row_prog, STATUS_ROW_PROG_H);
    lv_obj_align(row_prog, LV_ALIGN_TOP_MID, 0, STATUS_ROW_META_H + STATUS_ROW_GAP);
    lv_obj_set_flex_flow(row_prog, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_prog, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row_prog, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(row_prog, 2, LV_PART_MAIN);

    lv_obj_t *elapsed = lv_label_create(row_prog);
    lv_label_set_text(elapsed, "用时 00:00");
    lv_obj_set_width(elapsed, STATUS_ELAPSED_W);
    style_status_label(elapsed, UI_COLOR_TEXT_SEC, STATUS_ROW_PROG_H);
    g_status_elapsed = elapsed;

    lv_obj_t *bar = lv_bar_create(row_prog);
    lv_obj_set_flex_grow(bar, 1);
    lv_obj_set_height(bar, STATUS_BAR_H);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    laser_ui_style_status_bar(bar, UI_COLOR_ACCENT);
    g_status_bar = bar;

    lv_obj_t *pct = lv_label_create(row_prog);
    lv_label_set_text(pct, "0%");
    lv_obj_set_width(pct, STATUS_PCT_W);
    lv_obj_set_style_text_align(pct, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    style_status_label(pct, UI_COLOR_TEXT, STATUS_ROW_PROG_H);
    g_status_pct = pct;

    create_print_body_area(page);
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
