#include "page_pick.h"

#include "../laser_ui_layout.h"
#include "../laser_ui_widgets.h"
#include "../laser_ui_events.h"
#include "../pick/ui_pick_service.h"
static lv_obj_t *g_work_area = nullptr;
static lv_obj_t *g_head_dot = nullptr;
static lv_obj_t *g_cursor_dot = nullptr;
static lv_obj_t *g_coord_label = nullptr;
static lv_obj_t *g_status_label = nullptr;

static void emit_cb(lv_event_t *e)
{
    auto id = static_cast<laser_ui_event_id_t>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    laser_ui_events_emit(id);
}

static void work_area_pressing_cb(lv_event_t *e)
{
    if (g_work_area == nullptr) {
        return;
    }
    lv_indev_t *indev = lv_indev_active();
    if (indev == nullptr) {
        return;
    }
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    lv_area_t area;
    lv_obj_get_coords(g_work_area, &area);
    ui_pick_service_set_cursor_from_local_px(p.x - area.x1, p.y - area.y1);
}

static lv_obj_t *create_dot(lv_obj_t *parent, lv_color_t color, int size)
{
    lv_obj_t *dot = lv_obj_create(parent);
    lv_obj_set_size(dot, size, size);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(dot, color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
    return dot;
}

lv_obj_t *page_pick_create(lv_obj_t *parent)
{
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_set_size(page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(page, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(page, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(page, 6, LV_PART_MAIN);
    lv_obj_set_flex_flow(page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(page, 6, LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(page);
    lv_label_set_text(title, "选定原点 (42×42 mm)");
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT, LV_PART_MAIN);

    g_work_area = lv_obj_create(page);
    lv_obj_set_width(g_work_area, LV_PCT(100));
    lv_obj_set_flex_grow(g_work_area, 1);
    laser_ui_apply_panel_style(g_work_area, lv_color_hex(0x1A2030), 4);
    lv_obj_set_style_border_color(g_work_area, UI_COLOR_BORDER, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_work_area, 2, LV_PART_MAIN);
    lv_obj_add_flag(g_work_area, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(g_work_area, work_area_pressing_cb, LV_EVENT_PRESSING, nullptr);

    g_head_dot = create_dot(g_work_area, UI_COLOR_RUN, 10);
    g_cursor_dot = create_dot(g_work_area, UI_COLOR_ACCENT, 14);
    lv_obj_set_style_outline_color(g_cursor_dot, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_outline_width(g_cursor_dot, 2, LV_PART_MAIN);
    lv_obj_set_style_outline_opa(g_cursor_dot, LV_OPA_70, LV_PART_MAIN);

    g_coord_label = lv_label_create(page);
    lv_label_set_text(g_coord_label, "光标 X:0.0  Y:0.0 mm");
    lv_obj_set_style_text_color(g_coord_label, UI_COLOR_TEXT_DIM, LV_PART_MAIN);

    g_status_label = lv_label_create(page);
    lv_label_set_text(g_status_label, "");
    lv_obj_set_style_text_color(g_status_label, UI_COLOR_ACCENT, LV_PART_MAIN);

    lv_obj_t *btn_row = lv_obj_create(page);
    lv_obj_set_width(btn_row, LV_PCT(100));
    lv_obj_set_height(btn_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn_row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *btn_reset = laser_ui_create_button(btn_row, "回零", UI_COLOR_PANEL, lv_color_hex(0x2A2A2A));
    lv_obj_set_size(btn_reset, 100, 40);
    lv_obj_add_event_cb(btn_reset, emit_cb, LV_EVENT_CLICKED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(LASER_EVT_PICK_RESET)));

    lv_obj_t *btn_confirm = laser_ui_create_button(btn_row, "确定", UI_COLOR_NAV_ACTIVE, lv_color_hex(0x1A3050));
    lv_obj_set_size(btn_confirm, 100, 40);
    lv_obj_add_event_cb(btn_confirm, emit_cb, LV_EVENT_CLICKED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(LASER_EVT_PICK_CONFIRM)));

    return page;
}

void page_pick_on_show(void)
{
    ui_pick_service_on_page_show(g_work_area, g_head_dot, g_cursor_dot, g_coord_label, g_status_label);
    ui_pick_service_update_viewport_from_work_area();

    float cx = 0.0f;
    float cy = 0.0f;
    ui_pick_service_get_cursor_mm(&cx, &cy);
    ui_pick_service_set_cursor_mm(cx, cy);
}

void page_pick_on_hide(void)
{
    ui_pick_service_on_page_hide();
}
