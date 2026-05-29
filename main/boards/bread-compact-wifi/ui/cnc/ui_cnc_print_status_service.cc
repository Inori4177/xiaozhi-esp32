#include "ui_cnc_print_status_service.h"

#include "../laser_ui_layout.h"
#include "ui_cnc_motion_facade.h"
#include "ui_cnc_print_service.h"

#include <cstdio>
#include <cstring>

static lv_obj_t *g_badge = nullptr;
static lv_obj_t *g_elapsed = nullptr;
static lv_obj_t *g_eta = nullptr;
static lv_obj_t *g_bar = nullptr;
static lv_obj_t *g_pct = nullptr;
static lv_obj_t *g_pos_x = nullptr;
static lv_obj_t *g_pos_y = nullptr;

static lv_timer_t *g_refresh_timer = nullptr;

static ui_cnc_work_state_t g_last_state = UI_CNC_WORK_IDLE;
static uint8_t g_last_pct = 0xFF;
static uint32_t g_last_elapsed = 0xFFFFFFFF;
static uint32_t g_last_eta = 0xFFFFFFFF;
static bool g_last_has_eta = false;

static void format_mmss(uint32_t sec, char *buf, size_t len)
{
    const uint32_t m = sec / 60;
    const uint32_t s = sec % 60;
    snprintf(buf, len, "%02u:%02u", static_cast<unsigned>(m), static_cast<unsigned>(s));
}

static void apply_badge(ui_cnc_work_state_t state)
{
    if (g_badge == nullptr) {
        return;
    }
    if (state == g_last_state) {
        return;
    }

    g_last_state = state;
    switch (state) {
    case UI_CNC_WORK_RUNNING:
        lv_label_set_text(g_badge, " 运行 ");
        lv_obj_set_style_bg_color(g_badge, UI_COLOR_RUN, LV_PART_MAIN);
        lv_obj_set_style_text_color(g_badge, lv_color_white(), LV_PART_MAIN);
        break;
    case UI_CNC_WORK_JOGGING:
        lv_label_set_text(g_badge, " 点动 ");
        lv_obj_set_style_bg_color(g_badge, UI_COLOR_ACCENT, LV_PART_MAIN);
        lv_obj_set_style_text_color(g_badge, lv_color_white(), LV_PART_MAIN);
        break;
    case UI_CNC_WORK_PAUSED:
        lv_label_set_text(g_badge, " 暂停 ");
        lv_obj_set_style_bg_color(g_badge, UI_COLOR_PAUSE, LV_PART_MAIN);
        lv_obj_set_style_text_color(g_badge, lv_color_white(), LV_PART_MAIN);
        break;
    case UI_CNC_WORK_IDLE:
    default:
        lv_label_set_text(g_badge, " 空闲 ");
        lv_obj_set_style_bg_color(g_badge, UI_COLOR_STATUS_IDLE_BG, LV_PART_MAIN);
        lv_obj_set_style_text_color(g_badge, UI_COLOR_STATUS_IDLE_FG, LV_PART_MAIN);
        break;
    }
}

static void refresh_status_widgets(void)
{
    ui_cnc_print_status_t st = {};
    ui_cnc_print_service_get_status(&st);

    apply_badge(st.state);

    if (g_elapsed != nullptr && st.elapsed_sec != g_last_elapsed) {
        char time_buf[16];
        format_mmss(st.elapsed_sec, time_buf, sizeof(time_buf));
        char line[24];
        snprintf(line, sizeof(line), "用时 %s", time_buf);
        lv_label_set_text(g_elapsed, line);
        g_last_elapsed = st.elapsed_sec;
    }

    if (g_eta != nullptr &&
        (st.has_eta != g_last_has_eta || st.eta_sec != g_last_eta)) {
        char line[24];
        if (st.has_eta) {
            char time_buf[16];
            format_mmss(st.eta_sec, time_buf, sizeof(time_buf));
            snprintf(line, sizeof(line), "剩余 %s", time_buf);
        } else {
            snprintf(line, sizeof(line), "剩余 --:--");
        }
        lv_label_set_text(g_eta, line);
        g_last_has_eta = st.has_eta;
        g_last_eta = st.eta_sec;
    }

    if (st.progress_pct != g_last_pct) {
        if (g_bar != nullptr) {
            lv_bar_set_value(g_bar, st.progress_pct, LV_ANIM_OFF);
        }
        if (g_pct != nullptr) {
            char pct_buf[8];
            snprintf(pct_buf, sizeof(pct_buf), "%u%%", static_cast<unsigned>(st.progress_pct));
            lv_label_set_text(g_pct, pct_buf);
        }
        g_last_pct = st.progress_pct;
    }

    if (g_pos_x != nullptr || g_pos_y != nullptr) {
        float x = 0.0f;
        float y = 0.0f;
        ui_cnc_motion_facade_get_position_mm(&x, &y);

        char buf[16];
        if (g_pos_x != nullptr) {
            snprintf(buf, sizeof(buf), "X:%.1f", static_cast<double>(x));
            lv_label_set_text(g_pos_x, buf);
        }
        if (g_pos_y != nullptr) {
            snprintf(buf, sizeof(buf), "Y:%.1f", static_cast<double>(y));
            lv_label_set_text(g_pos_y, buf);
        }
    }
}

static void refresh_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    refresh_status_widgets();
}

static void reset_cached_status(void)
{
    g_last_state = static_cast<ui_cnc_work_state_t>(-1);
    g_last_pct = 0xFF;
    g_last_elapsed = 0xFFFFFFFF;
    g_last_eta = 0xFFFFFFFF;
    g_last_has_eta = false;
}

static void start_refresh_timer(void)
{
    if (g_refresh_timer == nullptr) {
        g_refresh_timer = lv_timer_create(refresh_timer_cb, 250, nullptr);
    } else {
        lv_timer_resume(g_refresh_timer);
    }
}

static void stop_refresh_timer(void)
{
    if (g_refresh_timer != nullptr) {
        lv_timer_pause(g_refresh_timer);
    }
}

void ui_cnc_print_status_service_init(void)
{
    /* Widget binding happens on page show. */
}

void ui_cnc_print_status_service_on_page_show(lv_obj_t *badge, lv_obj_t *elapsed, lv_obj_t *eta,
                                              lv_obj_t *bar, lv_obj_t *pct, lv_obj_t *pos_x,
                                              lv_obj_t *pos_y)
{
    g_badge = badge;
    g_elapsed = elapsed;
    g_eta = eta;
    g_bar = bar;
    g_pct = pct;
    g_pos_x = pos_x;
    g_pos_y = pos_y;

    reset_cached_status();
    refresh_status_widgets();
    start_refresh_timer();
}

void ui_cnc_print_status_service_on_page_hide(void)
{
    stop_refresh_timer();
    g_badge = nullptr;
    g_elapsed = nullptr;
    g_eta = nullptr;
    g_bar = nullptr;
    g_pct = nullptr;
    g_pos_x = nullptr;
    g_pos_y = nullptr;
}
