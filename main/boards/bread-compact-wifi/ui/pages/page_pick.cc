#include "page_pick.h"

#include "../laser_ui_layout.h"
#include "../laser_ui_widgets.h"
#include "../laser_ui_events.h"
#include "../pick/ui_pick_service.h"

static lv_obj_t *g_map_block = nullptr;
static lv_obj_t *g_title = nullptr;
static lv_obj_t *g_map_frame = nullptr;
static lv_obj_t *g_head_dot = nullptr;
static lv_obj_t *g_cursor_cross = nullptr;
static lv_obj_t *g_coord_x_label = nullptr;
static lv_obj_t *g_coord_y_label = nullptr;
static lv_obj_t *g_status_label = nullptr;

static constexpr int kCrosshairArmPx = 14;
static constexpr int kMapRailGap = 2;
static constexpr int kTitleMapGap = 2;
static constexpr int kSideBtnW = 74;
static constexpr int kSideBtnH = 42;
static constexpr int kCoordToBtnGap = 10;
static constexpr int kConfirmToResetGap = 18;

static void disable_scroll(lv_obj_t *obj)
{
    if (obj == nullptr) {
        return;
    }
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
}

static void emit_cb(lv_event_t *e)
{
    auto id = static_cast<laser_ui_event_id_t>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    laser_ui_events_emit(id);
}

static int clamp_local(int v, int max_v)
{
    if (v < 0) {
        return 0;
    }
    if (max_v > 0 && v > max_v) {
        return max_v;
    }
    return v;
}

static bool map_frame_local_point(int *local_x, int *local_y)
{
    if (g_map_frame == nullptr || local_x == nullptr || local_y == nullptr) {
        return false;
    }
    lv_indev_t *indev = lv_indev_active();
    if (indev == nullptr) {
        return false;
    }
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    lv_area_t area;
    lv_obj_get_coords(g_map_frame, &area);
    const int fw = lv_obj_get_width(g_map_frame) - 1;
    const int fh = lv_obj_get_height(g_map_frame) - 1;
    *local_x = clamp_local(p.x - area.x1, fw);
    *local_y = clamp_local(p.y - area.y1, fh);
    return true;
}

static void map_frame_touch_cb(lv_event_t *e)
{
    int lx = 0;
    int ly = 0;
    if (!map_frame_local_point(&lx, &ly)) {
        return;
    }

    switch (lv_event_get_code(e)) {
    case LV_EVENT_PRESSED:
        ui_pick_service_touch_begin(lx, ly);
        break;
    case LV_EVENT_PRESSING:
        ui_pick_service_set_cursor_from_local_px(lx, ly);
        break;
    case LV_EVENT_RELEASED:
        ui_pick_service_touch_end(lx, ly);
        break;
    default:
        break;
    }
}

static void relayout_map_frame(void)
{
    if (g_map_block == nullptr || g_map_frame == nullptr) {
        return;
    }

    const int block_w = lv_obj_get_width(g_map_block);
    const int block_h = lv_obj_get_height(g_map_block);
    const int title_h = (g_title != nullptr) ? lv_obj_get_height(g_title) : 0;
    const int avail_h = block_h - title_h - kTitleMapGap;
    int side = avail_h;
    if (side > block_w) {
        side = block_w;
    }
    if (side < 16) {
        return;
    }

    if (g_title != nullptr) {
        lv_obj_set_width(g_title, side);
    }

    if (lv_obj_get_width(g_map_frame) != side || lv_obj_get_height(g_map_frame) != side) {
        lv_obj_set_size(g_map_frame, side, side);
        lv_obj_align(g_map_frame, LV_ALIGN_BOTTOM_MID, 0, 0);
        ui_pick_service_update_viewport_from_map_frame();
    }

    float cx = 0.0f;
    float cy = 0.0f;
    ui_pick_service_get_cursor_mm(&cx, &cy);
    ui_pick_service_set_cursor_mm(cx, cy);
}

static void map_block_layout_cb(lv_event_t *e)
{
    (void)e;
    relayout_map_frame();
}

static lv_obj_t *create_head_dot(lv_obj_t *parent)
{
    lv_obj_t *dot = lv_obj_create(parent);
    lv_obj_set_size(dot, 10, 10);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(dot, UI_COLOR_RUN, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN);
    disable_scroll(dot);
    return dot;
}

static lv_obj_t *create_crosshair(lv_obj_t *parent, lv_color_t color, int arm_px)
{
    const int size = arm_px * 2 + 1;
    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_set_size(root, size, size);
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(root, 0, LV_PART_MAIN);
    disable_scroll(root);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *hline = lv_obj_create(root);
    lv_obj_set_size(hline, size, 2);
    lv_obj_set_style_bg_color(hline, color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(hline, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(hline, 0, LV_PART_MAIN);
    lv_obj_center(hline);
    disable_scroll(hline);

    lv_obj_t *vline = lv_obj_create(root);
    lv_obj_set_size(vline, 2, size);
    lv_obj_set_style_bg_color(vline, color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(vline, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(vline, 0, LV_PART_MAIN);
    lv_obj_center(vline);
    disable_scroll(vline);

    lv_obj_set_style_outline_color(root, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_outline_width(root, 1, LV_PART_MAIN);
    lv_obj_set_style_outline_opa(root, LV_OPA_60, LV_PART_MAIN);

    return root;
}

static lv_obj_t *create_coord_label(lv_obj_t *parent, const char *text)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_width(lbl, LV_SIZE_CONTENT);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(lbl, UI_COLOR_TEXT_DIM, LV_PART_MAIN);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    return lbl;
}

lv_obj_t *page_pick_create(lv_obj_t *parent)
{
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_set_size(page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(page, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(page, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(page, 2, LV_PART_MAIN);
    disable_scroll(page);

    lv_obj_t *body_row = lv_obj_create(page);
    lv_obj_set_size(body_row, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(body_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(body_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(body_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(body_row, kMapRailGap, LV_PART_MAIN);
    lv_obj_set_flex_flow(body_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(body_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    disable_scroll(body_row);

    g_map_block = lv_obj_create(body_row);
    lv_obj_set_flex_grow(g_map_block, 1);
    lv_obj_set_height(g_map_block, LV_PCT(100));
    lv_obj_set_style_bg_opa(g_map_block, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_map_block, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_map_block, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(g_map_block, kTitleMapGap, LV_PART_MAIN);
    lv_obj_set_flex_flow(g_map_block, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g_map_block, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    disable_scroll(g_map_block);
    lv_obj_add_event_cb(g_map_block, map_block_layout_cb, LV_EVENT_SIZE_CHANGED, nullptr);

    g_title = lv_label_create(g_map_block);
    lv_label_set_text(g_title, "选定原点 42×42mm");
    lv_obj_set_style_text_color(g_title, UI_COLOR_TEXT, LV_PART_MAIN);
    lv_obj_set_style_text_align(g_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_label_set_long_mode(g_title, LV_LABEL_LONG_CLIP);

    g_map_frame = lv_obj_create(g_map_block);
    laser_ui_apply_panel_style(g_map_frame, lv_color_hex(0x101820), 0);
    lv_obj_set_style_border_color(g_map_frame, UI_COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_map_frame, 2, LV_PART_MAIN);
    lv_obj_add_flag(g_map_frame, LV_OBJ_FLAG_CLICKABLE);
    disable_scroll(g_map_frame);
    lv_obj_add_event_cb(g_map_frame, map_frame_touch_cb, LV_EVENT_PRESSED, nullptr);
    lv_obj_add_event_cb(g_map_frame, map_frame_touch_cb, LV_EVENT_PRESSING, nullptr);
    lv_obj_add_event_cb(g_map_frame, map_frame_touch_cb, LV_EVENT_RELEASED, nullptr);

    g_head_dot = create_head_dot(g_map_frame);
    g_cursor_cross = create_crosshair(g_map_frame, UI_COLOR_ACCENT, kCrosshairArmPx);

    lv_obj_t *right_rail = lv_obj_create(body_row);
    lv_obj_set_width(right_rail, LV_SIZE_CONTENT);
    lv_obj_set_height(right_rail, LV_PCT(100));
    lv_obj_set_style_bg_opa(right_rail, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(right_rail, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(right_rail, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(right_rail, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(right_rail, 8, LV_PART_MAIN);
    lv_obj_set_flex_align(right_rail, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    disable_scroll(right_rail);

    g_coord_x_label = create_coord_label(right_rail, "X: 0.0 mm");
    g_coord_y_label = create_coord_label(right_rail, "Y: 0.0 mm");

    g_status_label = lv_label_create(right_rail);
    lv_label_set_text(g_status_label, "");
    lv_obj_set_width(g_status_label, LV_SIZE_CONTENT);
    lv_label_set_long_mode(g_status_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(g_status_label, UI_COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_add_flag(g_status_label, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *btn_confirm = laser_ui_create_button(right_rail, "确定", UI_COLOR_NAV_ACTIVE, lv_color_hex(0x1A3050));
    lv_obj_set_size(btn_confirm, kSideBtnW, kSideBtnH);
    lv_obj_set_ext_click_area(btn_confirm, 0);
    lv_obj_set_style_margin_top(btn_confirm, kCoordToBtnGap, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_confirm, emit_cb, LV_EVENT_CLICKED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(LASER_EVT_PICK_CONFIRM)));

    lv_obj_t *btn_reset = laser_ui_create_button(right_rail, "回零", UI_COLOR_PANEL, lv_color_hex(0x2A2A2A));
    lv_obj_set_size(btn_reset, kSideBtnW, kSideBtnH);
    lv_obj_set_ext_click_area(btn_reset, 0);
    lv_obj_set_style_margin_top(btn_reset, kConfirmToResetGap, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_reset, emit_cb, LV_EVENT_CLICKED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(LASER_EVT_PICK_RESET)));

    return page;
}

void page_pick_on_show(void)
{
    ui_pick_service_on_page_show(g_map_frame, g_head_dot, g_cursor_cross, g_coord_x_label, g_coord_y_label,
                                 g_status_label);
    relayout_map_frame();
}

void page_pick_on_hide(void)
{
    ui_pick_service_on_page_hide();
}
