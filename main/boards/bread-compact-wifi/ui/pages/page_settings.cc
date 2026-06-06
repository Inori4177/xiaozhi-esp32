#include "page_settings.h"

#include "../assets/laser_ui_images.h"
#include "../laser_ui_events.h"
#include "../laser_ui_layout.h"
#include "../laser_ui_widgets.h"
#include "../../laser_ui_state.h"

#include <esp_log.h>

static const char *TAG = "page_settings";

static void emit_cb(lv_event_t *e)
{
    auto id = static_cast<laser_ui_event_id_t>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    ESP_LOGI(TAG, "touch -> %s", laser_ui_event_name(id));
    laser_ui_events_emit(id);
}

static void power_slider_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        ESP_LOGI(TAG, "power slider changed");
        laser_ui_state_on_power_slider(e);
    }
}

static void speed_slider_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        ESP_LOGI(TAG, "speed slider changed");
        laser_ui_state_on_speed_slider(e);
    }
}

static void material_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, "material dropdown changed");
    laser_ui_state_on_material_changed(e);
    emit_cb(e);
}

static lv_obj_t *card(lv_obj_t *parent, int x, int y, int w, int h, int pad = 8)
{
    lv_obj_t *obj = laser_ui_create_hud_panel(parent, UI_COLOR_CARD, pad);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

static lv_obj_t *label(lv_obj_t *parent, const char *text, int x, int y,
                       lv_color_t color, int w = LV_SIZE_CONTENT)
{
    lv_obj_t *obj = lv_label_create(parent);
    lv_label_set_text(obj, text);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_width(obj, w);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(obj, color, LV_PART_MAIN);
    return obj;
}

static void icon(lv_obj_t *parent, const lv_image_dsc_t *src, int x, int y)
{
    lv_obj_t *img = lv_image_create(parent);
    lv_image_set_src(img, src);
    lv_obj_set_pos(img, x, y);
    lv_obj_set_size(img, 22, 22);
    lv_obj_remove_flag(img, LV_OBJ_FLAG_CLICKABLE);
}

lv_obj_t *page_settings_create(lv_obj_t *parent)
{
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_set_size(page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(page, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(page, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(page, 0, LV_PART_MAIN);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);

    const int x = 40;
    const int w = 344;

    lv_obj_t *material_card = card(page, x, 14, w, 50);
    icon(material_card, &ui_icon_material, 8, 13);
    label(material_card, "材料", 40, 16, UI_COLOR_TEXT_SEC, 80);
    lv_obj_t *material = lv_dropdown_create(material_card);
    lv_dropdown_set_options(material, "椴木 3mm\n亚克力\n皮革\n自定义");
    lv_obj_set_pos(material, 176, 8);
    lv_obj_set_size(material, 144, 32);
    lv_obj_set_style_bg_color(material, UI_COLOR_CARD_ALT, LV_PART_MAIN);
    lv_obj_set_style_border_color(material, UI_COLOR_LIGHT_BORDER, LV_PART_MAIN);
    lv_obj_set_style_radius(material, 8, LV_PART_MAIN);
    lv_obj_set_style_text_color(material, UI_COLOR_TEXT, LV_PART_MAIN);
    lv_obj_add_event_cb(material, material_cb, LV_EVENT_VALUE_CHANGED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(LASER_EVT_MATERIAL_CHANGED)));

    lv_obj_t *power_card = card(page, x, 74, w, 50);
    icon(power_card, &ui_icon_laser, 8, 13);
    label(power_card, "激光功率", 40, 16, UI_COLOR_TEXT_SEC, 88);
    lv_obj_t *power_val = label(power_card, "60%", 138, 16, UI_COLOR_STATUS_VALUE, 40);
    lv_obj_t *power = lv_slider_create(power_card);
    lv_obj_set_pos(power, 194, 21);
    lv_obj_set_size(power, 126, 10);
    lv_slider_set_range(power, 0, 10);
    lv_slider_set_value(power, 6, LV_ANIM_OFF);
    laser_ui_style_energy_slider(power, UI_COLOR_ACCENT);
    lv_obj_add_event_cb(power, power_slider_cb, LV_EVENT_VALUE_CHANGED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(LASER_EVT_POWER_CHANGED)));

    lv_obj_t *speed_card = card(page, x, 134, w, 50);
    icon(speed_card, &ui_icon_speed, 8, 13);
    label(speed_card, "运动速度", 40, 16, UI_COLOR_TEXT_SEC, 88);
    lv_obj_t *speed_val = label(speed_card, "1200", 136, 16, UI_COLOR_STATUS_VALUE, 48);
    lv_obj_t *speed = lv_slider_create(speed_card);
    lv_obj_set_pos(speed, 194, 21);
    lv_obj_set_size(speed, 126, 10);
    lv_slider_set_range(speed, 0, 5);
    lv_slider_set_value(speed, 1, LV_ANIM_OFF);
    laser_ui_style_energy_slider(speed, UI_COLOR_RUN);
    lv_obj_add_event_cb(speed, speed_slider_cb, LV_EVENT_VALUE_CHANGED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(LASER_EVT_SPEED_CHANGED)));

    laser_ui_state_bind_settings(material, power, power_val, speed, speed_val);

    lv_obj_t *apply = laser_ui_create_image_button(page, &ui_btn_apply_220x36,
                                                   "应用设置", lv_color_white());
    lv_obj_set_pos(apply, 102, 208);
    lv_obj_set_size(apply, 220, 36);
    lv_obj_add_event_cb(apply, emit_cb, LV_EVENT_CLICKED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(LASER_EVT_SETTINGS_APPLY)));

    return page;
}
