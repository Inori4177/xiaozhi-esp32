#include "page_settings.h"
#include "../laser_ui_layout.h"
#include "../laser_ui_widgets.h"
#include "../laser_ui_events.h"
#include "../../laser_ui_state.h"

static void emit_cb(lv_event_t *e)
{
    auto id = static_cast<laser_ui_event_id_t>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    laser_ui_events_emit(id);
}

static void power_slider_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    laser_ui_state_on_power_slider(e);
}

static void speed_slider_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    laser_ui_state_on_speed_slider(e);
}

static void material_cb(lv_event_t *e)
{
    laser_ui_state_on_material_changed(e);
    emit_cb(e);
}

static void style_value_label(lv_obj_t *label)
{
    lv_obj_set_style_text_color(label, UI_COLOR_STATUS_VALUE, LV_PART_MAIN);
}

lv_obj_t *page_settings_create(lv_obj_t *parent)
{
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_set_size(page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(page, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(page, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(page, 6, LV_PART_MAIN);
    lv_obj_set_flex_flow(page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(page, 8, LV_PART_MAIN);

    lv_obj_t *panel = lv_obj_create(page);
    lv_obj_set_width(panel, LV_PCT(100));
    lv_obj_set_flex_grow(panel, 1);
    laser_ui_apply_panel_style(panel, UI_COLOR_PANEL, 8);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(panel, 12, LV_PART_MAIN);

    lv_obj_t *mat_lbl = lv_label_create(panel);
    lv_label_set_text(mat_lbl, "材料设置");
    lv_obj_set_style_text_color(mat_lbl, UI_COLOR_TEXT_DIM, LV_PART_MAIN);
    lv_obj_t *material = lv_dropdown_create(panel);
    lv_dropdown_set_options(material, "Wood\nAcrylic\nLeather\nCustom");
    lv_obj_set_width(material, LV_PCT(100));
    lv_obj_add_event_cb(material, material_cb, LV_EVENT_VALUE_CHANGED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(LASER_EVT_MATERIAL_CHANGED)));

    lv_obj_t *pwr_lbl = lv_label_create(panel);
    lv_label_set_text(pwr_lbl, "激光功率");
    lv_obj_set_style_text_color(pwr_lbl, UI_COLOR_TEXT_DIM, LV_PART_MAIN);
    lv_obj_t *power_row = lv_obj_create(panel);
    lv_obj_set_width(power_row, LV_PCT(100));
    lv_obj_set_height(power_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(power_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(power_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(power_row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(power_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(power_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *power = lv_slider_create(power_row);
    lv_obj_set_width(power, LV_PCT(72));
    lv_slider_set_range(power, 0, 10);
    lv_slider_set_value(power, 5, LV_ANIM_OFF);
    lv_obj_add_event_cb(power, power_slider_cb, LV_EVENT_VALUE_CHANGED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(LASER_EVT_POWER_CHANGED)));
    lv_obj_t *power_val = lv_label_create(power_row);
    lv_label_set_text(power_val, "50%");
    style_value_label(power_val);
    lv_obj_set_width(power_val, 72);
    lv_obj_set_style_text_align(power_val, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);

    lv_obj_t *spd_lbl = lv_label_create(panel);
    lv_label_set_text(spd_lbl, "运动速度");
    lv_obj_set_style_text_color(spd_lbl, UI_COLOR_TEXT_DIM, LV_PART_MAIN);
    lv_obj_t *speed_row = lv_obj_create(panel);
    lv_obj_set_width(speed_row, LV_PCT(100));
    lv_obj_set_height(speed_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(speed_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(speed_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(speed_row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(speed_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(speed_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *speed = lv_slider_create(speed_row);
    lv_obj_set_width(speed, LV_PCT(72));
    lv_slider_set_range(speed, 0, 5);
    lv_slider_set_value(speed, 1, LV_ANIM_OFF);
    lv_obj_add_event_cb(speed, speed_slider_cb, LV_EVENT_VALUE_CHANGED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(LASER_EVT_SPEED_CHANGED)));
    lv_obj_t *speed_val = lv_label_create(speed_row);
    lv_label_set_text(speed_val, "100%");
    style_value_label(speed_val);
    lv_obj_set_width(speed_val, 72);
    lv_obj_set_style_text_align(speed_val, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);

    laser_ui_state_bind_settings(material, power, power_val, speed, speed_val);

    lv_obj_t *apply = laser_ui_create_button(page, "应用", UI_COLOR_ACCENT, lv_color_hex(0x1565C0));
    lv_obj_set_width(apply, LV_PCT(100));
    lv_obj_set_height(apply, 40);
    lv_obj_add_event_cb(apply, emit_cb, LV_EVENT_CLICKED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(LASER_EVT_SETTINGS_APPLY)));

    return page;
}
