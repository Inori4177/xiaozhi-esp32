#include "page_design.h"
#include "../laser_ui_layout.h"
#include "../laser_ui_widgets.h"
#include "../laser_ui_events.h"

static lv_obj_t *g_canvas_label = nullptr;
static lv_obj_t *g_tab_original = nullptr;
static lv_obj_t *g_tab_preview = nullptr;

static void emit_cb(lv_event_t *e)
{
    auto id = static_cast<laser_ui_event_id_t>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    laser_ui_events_emit(id);
}

static void tab_click_cb(lv_event_t *e)
{
    bool preview = lv_event_get_user_data(e) != nullptr;
    if (g_canvas_label != nullptr) {
        lv_label_set_text(g_canvas_label, preview ? "预览区（未实现）" : "原始图像区（未实现）");
    }
    if (g_tab_original != nullptr) {
        lv_obj_set_style_bg_color(g_tab_original, preview ? UI_COLOR_PANEL : UI_COLOR_NAV_ACTIVE, LV_PART_MAIN);
    }
    if (g_tab_preview != nullptr) {
        lv_obj_set_style_bg_color(g_tab_preview, preview ? UI_COLOR_NAV_ACTIVE : UI_COLOR_PANEL, LV_PART_MAIN);
    }
    laser_ui_events_emit(preview ? LASER_EVT_DESIGN_TAB_PREVIEW : LASER_EVT_DESIGN_TAB_ORIGINAL);
}

lv_obj_t *page_design_create(lv_obj_t *parent)
{
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_set_size(page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(page, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(page, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(page, 4, LV_PART_MAIN);
    lv_obj_set_flex_flow(page, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(page, 4, LV_PART_MAIN);

    lv_obj_t *tools = lv_obj_create(page);
    lv_obj_set_width(tools, 64);
    lv_obj_set_height(tools, LV_PCT(100));
    laser_ui_apply_panel_style(tools, UI_COLOR_NAV, 4);
    lv_obj_set_flex_flow(tools, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(tools, 4, LV_PART_MAIN);

    const struct {
        const char *label;
        laser_ui_event_id_t id;
    } tool_btns[] = {
        {"导入", LASER_EVT_DESIGN_IMPORT},
        {"裁剪", LASER_EVT_DESIGN_CROP},
        {"框选", LASER_EVT_DESIGN_SELECT},
        {"定标", LASER_EVT_DESIGN_CALIB},
    };

    for (const auto &t : tool_btns) {
        lv_obj_t *btn = laser_ui_create_button(tools, t.label, UI_COLOR_PANEL, lv_color_hex(0x2A2A2A));
        lv_obj_set_size(btn, 52, 44);
        lv_obj_add_state(btn, LV_STATE_DISABLED);
        lv_obj_add_event_cb(btn, emit_cb, LV_EVENT_CLICKED,
                            reinterpret_cast<void *>(static_cast<intptr_t>(t.id)));
    }

    lv_obj_t *right = lv_obj_create(page);
    lv_obj_set_flex_grow(right, 1);
    lv_obj_set_height(right, LV_PCT(100));
    lv_obj_set_style_bg_opa(right, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(right, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(right, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(right, 4, LV_PART_MAIN);

    lv_obj_t *tabs = lv_obj_create(right);
    lv_obj_set_width(tabs, LV_PCT(100));
    lv_obj_set_height(tabs, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(tabs, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(tabs, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(tabs, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(tabs, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(tabs, 4, LV_PART_MAIN);

    g_tab_original = laser_ui_create_tab_button(tabs, "原始图像", true);
    lv_obj_add_event_cb(g_tab_original, tab_click_cb, LV_EVENT_CLICKED, nullptr);
    g_tab_preview = laser_ui_create_tab_button(tabs, "预览", false);
    lv_obj_add_event_cb(g_tab_preview, tab_click_cb, LV_EVENT_CLICKED, reinterpret_cast<void *>(1));

    lv_obj_t *canvas = lv_obj_create(right);
    lv_obj_set_width(canvas, LV_PCT(100));
    lv_obj_set_flex_grow(canvas, 1);
    laser_ui_apply_panel_style(canvas, lv_color_hex(0x2A2A2A), 4);
    g_canvas_label = lv_label_create(canvas);
    lv_label_set_text(g_canvas_label, "原始图像区（未实现）");
    lv_obj_set_style_text_color(g_canvas_label, UI_COLOR_TEXT_DIM, LV_PART_MAIN);
    lv_obj_center(g_canvas_label);

    return page;
}
