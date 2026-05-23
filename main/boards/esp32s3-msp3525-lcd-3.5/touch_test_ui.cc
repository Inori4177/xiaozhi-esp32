#include "touch_test_ui.h"
#include "ft6336_touch.h"

#include <esp_log.h>
#include <esp_lvgl_port.h>
#include <lvgl.h>

#define TAG "touch_test"

static lv_obj_t *g_status_label = nullptr;
static lv_obj_t *g_coord_label = nullptr;
static int g_press_count = 0;

static void btn_click_cb(lv_event_t *e)
{
    (void)e;
    g_press_count++;
    if (g_status_label != nullptr) {
        lv_label_set_text_fmt(g_status_label, "Press count: %d", g_press_count);
    }
    ESP_LOGI(TAG, "Button clicked, count=%d", g_press_count);
}

static void coord_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (g_coord_label == nullptr) {
        return;
    }

    ft6336_touch_read();
    int x = 0;
    int y = 0;
    if (ft6336_touch_get_point(&x, &y)) {
        lv_label_set_text_fmt(g_coord_label, "Touch: (%d, %d)", x, y);
    } else {
        lv_label_set_text(g_coord_label, "Touch: (released)");
    }
}

void msp3525_show_touch_test_ui(void)
{
    if (!lvgl_port_lock(0)) {
        ESP_LOGE(TAG, "Failed to lock LVGL port");
        return;
    }

    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1E3A5F), 0);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "FT6336 Touch Test");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 24);

    lv_obj_t *hint = lv_label_create(scr);
    lv_label_set_text(hint, "Tap the button below");
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 52);

    lv_obj_t *btn = lv_button_create(scr);
    lv_obj_set_size(btn, 240, 72);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, -24);
    lv_obj_add_event_cb(btn, btn_click_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Tap Me");
    lv_obj_center(btn_label);

    g_status_label = lv_label_create(scr);
    lv_label_set_text(g_status_label, "Press count: 0");
    lv_obj_align(g_status_label, LV_ALIGN_CENTER, 0, 56);

    g_coord_label = lv_label_create(scr);
    lv_label_set_text(g_coord_label, "Touch: (released)");
    lv_obj_align(g_coord_label, LV_ALIGN_BOTTOM_MID, 0, -36);

    lv_timer_create(coord_timer_cb, 50, nullptr);

    lvgl_port_unlock();
    ESP_LOGI(TAG, "Touch test UI ready (480x320 landscape)");
}
