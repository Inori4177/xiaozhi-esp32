#include "page_pick.h"



#include "../assets/laser_ui_images.h"

#include "../laser_ui_layout.h"

#include "../laser_ui_widgets.h"

#include "../laser_ui_events.h"

#include "../pick/ui_pick_keypad_input.h"

#include "../pick/ui_pick_service.h"

#include <cstdint>
#include <cstdio>



static lv_obj_t *g_map_block = nullptr;

static lv_obj_t *g_title = nullptr;

static lv_obj_t *g_map_frame = nullptr;

static lv_obj_t *g_head_dot = nullptr;

static lv_obj_t *g_cursor_cross = nullptr;

static lv_obj_t *g_coord_x_box = nullptr;

static lv_obj_t *g_coord_y_box = nullptr;

static lv_obj_t *g_coord_x_label = nullptr;

static lv_obj_t *g_coord_y_label = nullptr;

static lv_obj_t *g_status_label = nullptr;

static lv_obj_t *g_btn_col = nullptr;

static lv_obj_t *g_keyboard_panel = nullptr;

static lv_obj_t *g_keyboard_feedback[12] = {};

static lv_obj_t *g_keyboard_hitboxes[12] = {};



static constexpr int kCrosshairArmPx = 14;

static constexpr int kMapRailGap = 3;

static constexpr int kTitleMapGap = 2;

static constexpr int kRightRailW = 120;

static constexpr int kCoordBoxW = 120;

static constexpr int kCoordBoxH = 28;

static constexpr int kKeyboardW = 140;

static constexpr int kKeyboardH = 130;

static constexpr int kKeyboardGapFromMap = 4;

static constexpr int kKeyboardGapBelowCoord = 8;

static constexpr int kKeyboardBottomMargin = 4;

static constexpr int kCoordMaxMmTenths = 420;
static constexpr float kCoordMaxMm = kCoordMaxMmTenths / 10.0f;

enum class PickEditAxis : uint8_t {
    None = 0,
    X,
    Y,
};

struct PickKeyDef {
    char token;
    int x;
    int y;
    int w;
    int h;
};

static constexpr PickKeyDef kKeyboardKeys[] = {
    {'1', 0, 0, 46, 32},
    {'2', 46, 0, 48, 32},
    {'3', 94, 0, 46, 32},
    {'4', 0, 32, 46, 32},
    {'5', 46, 32, 48, 32},
    {'6', 94, 32, 46, 32},
    {'7', 0, 64, 46, 32},
    {'8', 46, 64, 48, 32},
    {'9', 94, 64, 46, 32},
    {'0', 0, 96, 46, 34},
    {'.', 46, 96, 48, 34},
    {'\b', 94, 96, 46, 34},
};

static PickEditAxis g_active_edit_axis = PickEditAxis::None;
static char g_edit_buffer[16] = {0};

static void stop_coord_editing(bool commit_value);
static void refresh_editing_label(void);
static void keyboard_key_event_cb(lv_event_t *e);
static void relayout_keyboard_panel(void);

static const char *key_text_for_token(char token)
{
    switch (token) {
    case '\b':
        return "x";
    case '.':
        return ".";
    case '0':
        return "0";
    case '1':
        return "1";
    case '2':
        return "2";
    case '3':
        return "3";
    case '4':
        return "4";
    case '5':
        return "5";
    case '6':
        return "6";
    case '7':
        return "7";
    case '8':
        return "8";
    case '9':
        return "9";
    default:
        return "";
    }
}



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

        stop_coord_editing(false);
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

    relayout_keyboard_panel();

}



static void map_block_layout_cb(lv_event_t *e)

{

    (void)e;

    relayout_map_frame();

}



static void relayout_keyboard_panel(void)

{

    if (g_keyboard_panel == nullptr || g_map_frame == nullptr || g_coord_y_box == nullptr) {
        return;
    }

    lv_obj_t *page = lv_obj_get_parent(g_keyboard_panel);
    if (page == nullptr) {
        return;
    }

    lv_area_t page_area;
    lv_area_t map_area;
    lv_area_t coord_y_area;
    lv_obj_get_coords(page, &page_area);
    lv_obj_get_coords(g_map_frame, &map_area);
    lv_obj_get_coords(g_coord_y_box, &coord_y_area);

    int x = map_area.x2 - page_area.x1 + 1 + kKeyboardGapFromMap;
    const int max_x = lv_obj_get_width(page) - kKeyboardW;
    if (x > max_x) {
        x = max_x;
    }
    if (x < 0) {
        x = 0;
    }

    int y = map_area.y1 - page_area.y1 + (lv_obj_get_height(g_map_frame) - kKeyboardH) / 2;
    const int min_y = coord_y_area.y2 - page_area.y1 + 1 + kKeyboardGapBelowCoord;
    if (y < min_y) {
        y = min_y;
    }
    const int max_y = lv_obj_get_height(page) - kKeyboardH - kKeyboardBottomMargin;
    if (y > max_y) {
        y = max_y;
    }
    if (y < 0) {
        y = 0;
    }

    lv_obj_set_pos(g_keyboard_panel, x, y);
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

    lv_obj_set_width(lbl, LV_PCT(100));

    lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);

    lv_obj_set_style_text_color(lbl, UI_COLOR_TEXT, LV_PART_MAIN);

    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);

    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 8, 0);

    return lbl;

}



static lv_obj_t *create_coord_box(lv_obj_t *parent, lv_obj_t **out_label, const char *text, PickEditAxis axis)

{

    lv_obj_t *box = lv_obj_create(parent);

    lv_obj_set_size(box, kCoordBoxW, kCoordBoxH);

    lv_obj_set_style_radius(box, 8, LV_PART_MAIN);

    lv_obj_set_style_bg_color(box, UI_COLOR_CARD, LV_PART_MAIN);

    lv_obj_set_style_bg_opa(box, LV_OPA_80, LV_PART_MAIN);

    lv_obj_set_style_border_color(box, UI_COLOR_SOFT_MINT, LV_PART_MAIN);

    lv_obj_set_style_border_width(box, 1, LV_PART_MAIN);

    lv_obj_set_style_outline_width(box, 0, LV_PART_MAIN);

    lv_obj_set_style_pad_all(box, 0, LV_PART_MAIN);

    disable_scroll(box);

    lv_obj_add_flag(box, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *label = create_coord_label(box, text);

    if (out_label != nullptr) {
        *out_label = label;
    }

    lv_obj_add_event_cb(box, [](lv_event_t *e) {
        const auto axis_value =
            static_cast<PickEditAxis>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
        if (axis_value == PickEditAxis::None) {
            return;
        }
        if (g_active_edit_axis == axis_value) {
            float x_mm = 0.0f;
            float y_mm = 0.0f;
            ui_pick_service_get_cursor_mm(&x_mm, &y_mm);
            float next_mm = 0.0f;
            const bool valid = ui_pick_keypad_commit_value(g_edit_buffer, 0.0f, kCoordMaxMm, &next_mm);
            if (valid) {
                if (axis_value == PickEditAxis::X) {
                    x_mm = next_mm;
                } else {
                    y_mm = next_mm;
                }
                ui_pick_service_set_cursor_mm(x_mm, y_mm);
            } else {
                ui_pick_service_set_cursor_mm(x_mm, y_mm);
            }
            g_active_edit_axis = PickEditAxis::None;
            if (g_keyboard_panel != nullptr) {
                lv_obj_add_flag(g_keyboard_panel, LV_OBJ_FLAG_HIDDEN);
            }
            if (g_btn_col != nullptr) {
                lv_obj_remove_flag(g_btn_col, LV_OBJ_FLAG_HIDDEN);
            }
        } else {
            float x_mm = 0.0f;
            float y_mm = 0.0f;
            ui_pick_service_get_cursor_mm(&x_mm, &y_mm);
            if (g_active_edit_axis != PickEditAxis::None) {
                float next_mm = 0.0f;
                const bool valid = ui_pick_keypad_commit_value(g_edit_buffer, 0.0f, kCoordMaxMm, &next_mm);
                if (valid) {
                    if (g_active_edit_axis == PickEditAxis::X) {
                        x_mm = next_mm;
                    } else {
                        y_mm = next_mm;
                    }
                    ui_pick_service_set_cursor_mm(x_mm, y_mm);
                }
            }
            const float current_mm = (axis_value == PickEditAxis::X) ? x_mm : y_mm;
            ui_pick_keypad_format_initial(current_mm, g_edit_buffer, sizeof(g_edit_buffer));
            g_active_edit_axis = axis_value;
            if (g_keyboard_panel != nullptr) {
                lv_obj_remove_flag(g_keyboard_panel, LV_OBJ_FLAG_HIDDEN);
            }
            if (g_btn_col != nullptr) {
                lv_obj_add_flag(g_btn_col, LV_OBJ_FLAG_HIDDEN);
            }
        }

        const bool edit_x = g_active_edit_axis == PickEditAxis::X;
        const bool edit_y = g_active_edit_axis == PickEditAxis::Y;

        if (g_coord_x_box != nullptr) {
            lv_obj_set_style_border_color(g_coord_x_box, edit_x ? UI_COLOR_ACCENT : UI_COLOR_SOFT_MINT, LV_PART_MAIN);
            lv_obj_set_style_border_width(g_coord_x_box, edit_x ? 2 : 1, LV_PART_MAIN);
        }

        if (g_coord_y_box != nullptr) {
            lv_obj_set_style_border_color(g_coord_y_box, edit_y ? UI_COLOR_ACCENT : UI_COLOR_SOFT_MINT, LV_PART_MAIN);
            lv_obj_set_style_border_width(g_coord_y_box, edit_y ? 2 : 1, LV_PART_MAIN);
        }

        if (g_active_edit_axis == PickEditAxis::X && g_coord_x_label != nullptr) {
            char buf[24];
            std::snprintf(buf, sizeof(buf), "X:%s", g_edit_buffer[0] != '\0' ? g_edit_buffer : "_");
            lv_label_set_text(g_coord_x_label, buf);
        } else if (g_active_edit_axis != PickEditAxis::X) {
            float x_mm = 0.0f;
            float y_mm = 0.0f;
            ui_pick_service_get_cursor_mm(&x_mm, &y_mm);
            ui_pick_service_set_cursor_mm(x_mm, y_mm);
        }

        if (g_active_edit_axis == PickEditAxis::Y && g_coord_y_label != nullptr) {
            char buf[24];
            std::snprintf(buf, sizeof(buf), "Y:%s", g_edit_buffer[0] != '\0' ? g_edit_buffer : "_");
            lv_label_set_text(g_coord_y_label, buf);
        } else if (g_active_edit_axis != PickEditAxis::Y) {
            float x_mm = 0.0f;
            float y_mm = 0.0f;
            ui_pick_service_get_cursor_mm(&x_mm, &y_mm);
            ui_pick_service_set_cursor_mm(x_mm, y_mm);
        }
    }, LV_EVENT_CLICKED, reinterpret_cast<void *>(static_cast<intptr_t>(axis)));

    return box;

}



static void stop_coord_editing(bool commit_value)

{

    if (g_active_edit_axis == PickEditAxis::None) {
        return;
    }

    float x_mm = 0.0f;
    float y_mm = 0.0f;
    ui_pick_service_get_cursor_mm(&x_mm, &y_mm);

    if (commit_value) {
        float next_mm = 0.0f;
        if (ui_pick_keypad_commit_value(g_edit_buffer, 0.0f, kCoordMaxMm, &next_mm)) {
            if (g_active_edit_axis == PickEditAxis::X) {
                x_mm = next_mm;
            } else {
                y_mm = next_mm;
            }
        }
    }

    g_active_edit_axis = PickEditAxis::None;

    if (g_keyboard_panel != nullptr) {
        lv_obj_add_flag(g_keyboard_panel, LV_OBJ_FLAG_HIDDEN);
    }

    if (g_btn_col != nullptr) {
        lv_obj_remove_flag(g_btn_col, LV_OBJ_FLAG_HIDDEN);
    }

    if (g_coord_x_box != nullptr) {
        lv_obj_set_style_border_color(g_coord_x_box, UI_COLOR_SOFT_MINT, LV_PART_MAIN);
        lv_obj_set_style_border_width(g_coord_x_box, 1, LV_PART_MAIN);
    }

    if (g_coord_y_box != nullptr) {
        lv_obj_set_style_border_color(g_coord_y_box, UI_COLOR_SOFT_MINT, LV_PART_MAIN);
        lv_obj_set_style_border_width(g_coord_y_box, 1, LV_PART_MAIN);
    }

    ui_pick_service_set_cursor_mm(x_mm, y_mm);

}



static void refresh_editing_label(void)

{

    if (g_active_edit_axis == PickEditAxis::X && g_coord_x_label != nullptr) {
        char buf[24];
        std::snprintf(buf, sizeof(buf), "X:%s", g_edit_buffer[0] != '\0' ? g_edit_buffer : "_");
        lv_label_set_text(g_coord_x_label, buf);
    } else if (g_active_edit_axis == PickEditAxis::Y && g_coord_y_label != nullptr) {
        char buf[24];
        std::snprintf(buf, sizeof(buf), "Y:%s", g_edit_buffer[0] != '\0' ? g_edit_buffer : "_");
        lv_label_set_text(g_coord_y_label, buf);
    }

}



static void keyboard_key_event_cb(lv_event_t *e)

{

    const int key_index = static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    if (key_index < 0 || key_index >= static_cast<int>(sizeof(kKeyboardKeys) / sizeof(kKeyboardKeys[0]))) {
        return;
    }

    lv_obj_t *feedback = g_keyboard_feedback[key_index];
    switch (lv_event_get_code(e)) {
    case LV_EVENT_PRESSED:
        if (feedback != nullptr) {
            lv_obj_remove_flag(feedback, LV_OBJ_FLAG_HIDDEN);
        }
        break;
    case LV_EVENT_RELEASED:
        if (feedback != nullptr) {
            lv_obj_add_flag(feedback, LV_OBJ_FLAG_HIDDEN);
        }
        if (g_active_edit_axis == PickEditAxis::None) {
            break;
        }
        if (ui_pick_keypad_apply_token(g_edit_buffer, sizeof(g_edit_buffer), kKeyboardKeys[key_index].token)) {
            refresh_editing_label();
        }
        break;
    case LV_EVENT_PRESS_LOST:
        if (feedback != nullptr) {
            lv_obj_add_flag(feedback, LV_OBJ_FLAG_HIDDEN);
        }
        break;
    default:
        break;
    }

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

    lv_obj_set_flex_align(g_map_block, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    disable_scroll(g_map_block);

    lv_obj_add_event_cb(g_map_block, map_block_layout_cb, LV_EVENT_SIZE_CHANGED, nullptr);



    g_title = lv_label_create(g_map_block);

    lv_label_set_text(g_title, "选定原点 42×42mm");

    lv_obj_set_style_text_color(g_title, UI_COLOR_TEXT, LV_PART_MAIN);

    lv_obj_set_style_text_align(g_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    lv_label_set_long_mode(g_title, LV_LABEL_LONG_CLIP);



    g_map_frame = lv_obj_create(g_map_block);

    laser_ui_apply_panel_style(g_map_frame, UI_COLOR_CARD_ALT, 0);

    lv_obj_set_style_border_color(g_map_frame, UI_COLOR_SOFT_MINT, LV_PART_MAIN);

    lv_obj_set_style_border_width(g_map_frame, 2, LV_PART_MAIN);

    lv_obj_add_flag(g_map_frame, LV_OBJ_FLAG_CLICKABLE);

    disable_scroll(g_map_frame);

    lv_obj_add_event_cb(g_map_frame, map_frame_touch_cb, LV_EVENT_PRESSED, nullptr);

    lv_obj_add_event_cb(g_map_frame, map_frame_touch_cb, LV_EVENT_PRESSING, nullptr);

    lv_obj_add_event_cb(g_map_frame, map_frame_touch_cb, LV_EVENT_RELEASED, nullptr);



    lv_obj_t *grid = lv_image_create(g_map_frame);

    lv_image_set_src(grid, &ui_pick_grid_240);

    lv_obj_center(grid);

    lv_obj_move_background(grid);

    lv_obj_remove_flag(grid, LV_OBJ_FLAG_CLICKABLE);

    laser_ui_add_map_grid(g_map_frame, UI_COLOR_ACCENT);



    g_head_dot = create_head_dot(g_map_frame);

    g_cursor_cross = create_crosshair(g_map_frame, UI_COLOR_ACCENT, kCrosshairArmPx);



    lv_obj_t *right_rail = lv_obj_create(body_row);

    lv_obj_set_width(right_rail, kRightRailW);

    lv_obj_set_height(right_rail, LV_PCT(100));

    lv_obj_set_style_bg_opa(right_rail, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_obj_set_style_border_width(right_rail, 0, LV_PART_MAIN);

    lv_obj_set_style_pad_all(right_rail, 0, LV_PART_MAIN);

    lv_obj_set_flex_flow(right_rail, LV_FLEX_FLOW_COLUMN);

    lv_obj_set_style_pad_row(right_rail, UI_PICK_RAIL_GAP, LV_PART_MAIN);

    lv_obj_set_flex_align(right_rail, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    disable_scroll(right_rail);



    g_coord_x_box = create_coord_box(right_rail, &g_coord_x_label, "X: 0.0 mm", PickEditAxis::X);

    g_coord_y_box = create_coord_box(right_rail, &g_coord_y_label, "Y: 0.0 mm", PickEditAxis::Y);



    lv_obj_t *status_slot = lv_obj_create(right_rail);

    lv_obj_set_size(status_slot, kRightRailW, UI_PICK_STATUS_H);

    lv_obj_set_style_bg_opa(status_slot, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_obj_set_style_border_width(status_slot, 0, LV_PART_MAIN);

    lv_obj_set_style_pad_all(status_slot, 0, LV_PART_MAIN);

    disable_scroll(status_slot);

    lv_obj_clear_flag(status_slot, LV_OBJ_FLAG_CLICKABLE);



    g_status_label = lv_label_create(status_slot);

    lv_label_set_text(g_status_label, "");

    lv_obj_set_width(g_status_label, kRightRailW);

    lv_label_set_long_mode(g_status_label, LV_LABEL_LONG_CLIP);

    lv_obj_set_style_text_color(g_status_label, UI_COLOR_ACCENT, LV_PART_MAIN);

    lv_obj_set_style_text_align(g_status_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);

    lv_obj_align(g_status_label, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_add_flag(g_status_label, LV_OBJ_FLAG_HIDDEN);



    g_btn_col = lv_obj_create(right_rail);

    lv_obj_set_width(g_btn_col, kRightRailW);

    lv_obj_set_height(g_btn_col, LV_SIZE_CONTENT);

    lv_obj_set_style_bg_opa(g_btn_col, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_obj_set_style_border_width(g_btn_col, 0, LV_PART_MAIN);

    lv_obj_set_style_pad_all(g_btn_col, 0, LV_PART_MAIN);

    lv_obj_set_style_pad_row(g_btn_col, UI_PICK_BTN_GAP, LV_PART_MAIN);
    lv_obj_set_flex_flow(g_btn_col, LV_FLEX_FLOW_COLUMN);

    lv_obj_set_flex_align(g_btn_col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    disable_scroll(g_btn_col);

    lv_obj_clear_flag(g_btn_col, LV_OBJ_FLAG_CLICKABLE);



    lv_obj_t *btn_confirm = laser_ui_create_png_button(g_btn_col, &ui_btn_pick_settle_90x45,

                                                       UI_PICK_BTN_W, UI_PICK_BTN_H,

                                                       UI_PICK_BTN_H / 2, UI_PICK_BTN_EXT_CLICK);

    lv_obj_add_event_cb(btn_confirm, emit_cb, LV_EVENT_CLICKED,

                        reinterpret_cast<void *>(static_cast<intptr_t>(LASER_EVT_PICK_CONFIRM)));



    lv_obj_t *btn_reset = laser_ui_create_png_button(g_btn_col, &ui_btn_pick_homing_90x45,

                                                     UI_PICK_BTN_W, UI_PICK_BTN_H,

                                                     UI_PICK_BTN_H / 2, UI_PICK_BTN_EXT_CLICK);

    lv_obj_add_event_cb(btn_reset, emit_cb, LV_EVENT_CLICKED,

                        reinterpret_cast<void *>(static_cast<intptr_t>(LASER_EVT_PICK_RESET)));



    g_keyboard_panel = lv_obj_create(page);

    lv_obj_set_size(g_keyboard_panel, kKeyboardW, kKeyboardH);

    lv_obj_set_style_radius(g_keyboard_panel, 8, LV_PART_MAIN);
    lv_obj_set_style_bg_color(g_keyboard_panel, lv_color_hex(0xD7D9DD), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(g_keyboard_panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(g_keyboard_panel, lv_color_hex(0xC7CCD1), LV_PART_MAIN);
    lv_obj_set_style_border_width(g_keyboard_panel, 1, LV_PART_MAIN);

    lv_obj_set_style_pad_all(g_keyboard_panel, 0, LV_PART_MAIN);

    disable_scroll(g_keyboard_panel);

    lv_obj_add_flag(g_keyboard_panel, LV_OBJ_FLAG_HIDDEN);

    for (size_t i = 0; i < sizeof(kKeyboardKeys) / sizeof(kKeyboardKeys[0]); ++i) {
        const PickKeyDef &key = kKeyboardKeys[i];

        g_keyboard_hitboxes[i] = lv_obj_create(g_keyboard_panel);
        lv_obj_set_pos(g_keyboard_hitboxes[i], key.x, key.y);
        lv_obj_set_size(g_keyboard_hitboxes[i], key.w, key.h);
        lv_obj_set_style_radius(g_keyboard_hitboxes[i], 5, LV_PART_MAIN);
        lv_obj_set_style_bg_color(g_keyboard_hitboxes[i], lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(g_keyboard_hitboxes[i], LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(g_keyboard_hitboxes[i], lv_color_hex(0xD3D7DB), LV_PART_MAIN);
        lv_obj_set_style_border_width(g_keyboard_hitboxes[i], 1, LV_PART_MAIN);
        lv_obj_set_style_pad_all(g_keyboard_hitboxes[i], 0, LV_PART_MAIN);
        disable_scroll(g_keyboard_hitboxes[i]);
        lv_obj_add_flag(g_keyboard_hitboxes[i], LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *key_label = lv_label_create(g_keyboard_hitboxes[i]);
        lv_label_set_text(key_label, key_text_for_token(key.token));
        lv_obj_set_style_text_color(key_label, key.token == '\b' ? lv_color_hex(0x9EA6AD) : UI_COLOR_TEXT, LV_PART_MAIN);
        lv_obj_set_style_text_align(key_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_center(key_label);

        g_keyboard_feedback[i] = lv_obj_create(g_keyboard_panel);
        lv_obj_set_pos(g_keyboard_feedback[i], key.x, key.y);
        lv_obj_set_size(g_keyboard_feedback[i], key.w, key.h);
        lv_obj_set_style_radius(g_keyboard_feedback[i], 5, LV_PART_MAIN);
        lv_obj_set_style_bg_color(g_keyboard_feedback[i], UI_COLOR_ACCENT, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(g_keyboard_feedback[i], LV_OPA_20, LV_PART_MAIN);
        lv_obj_set_style_border_color(g_keyboard_feedback[i], UI_COLOR_AQUA, LV_PART_MAIN);
        lv_obj_set_style_border_width(g_keyboard_feedback[i], 1, LV_PART_MAIN);
        lv_obj_set_style_border_opa(g_keyboard_feedback[i], LV_OPA_40, LV_PART_MAIN);
        disable_scroll(g_keyboard_feedback[i]);
        lv_obj_clear_flag(g_keyboard_feedback[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(g_keyboard_feedback[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(g_keyboard_feedback[i]);

        lv_obj_add_event_cb(g_keyboard_hitboxes[i], keyboard_key_event_cb, LV_EVENT_PRESSED,
                            reinterpret_cast<void *>(static_cast<intptr_t>(i)));
        lv_obj_add_event_cb(g_keyboard_hitboxes[i], keyboard_key_event_cb, LV_EVENT_RELEASED,
                            reinterpret_cast<void *>(static_cast<intptr_t>(i)));
        lv_obj_add_event_cb(g_keyboard_hitboxes[i], keyboard_key_event_cb, LV_EVENT_PRESS_LOST,
                            reinterpret_cast<void *>(static_cast<intptr_t>(i)));
    }

    return page;

}



void page_pick_on_show(void)

{

    ui_pick_service_on_page_show(g_map_frame, g_head_dot, g_cursor_cross, g_coord_x_label, g_coord_y_label,

                                 g_status_label);

    g_active_edit_axis = PickEditAxis::None;
    g_edit_buffer[0] = '\0';
    if (g_keyboard_panel != nullptr) {
        lv_obj_add_flag(g_keyboard_panel, LV_OBJ_FLAG_HIDDEN);
    }
    if (g_btn_col != nullptr) {
        lv_obj_remove_flag(g_btn_col, LV_OBJ_FLAG_HIDDEN);
    }

    relayout_map_frame();
    relayout_keyboard_panel();

}



void page_pick_on_hide(void)

{

    stop_coord_editing(false);
    ui_pick_service_on_page_hide();

}


