#include "laser_ui_splash.h"
#include "laser_ui_layout.h"
#include "laser_ui.h"

#include "display.h"
#include "boards/common/board_custom_ui.h"

#include <esp_log.h>
#include <cstring>

static const char *TAG = "laser_ui_splash";

LV_FONT_DECLARE(BUILTIN_TEXT_FONT);

// One visible glyph per entry (left-to-right typewriter order). UTF-8 bytes, no u8"" (C++20 char8_t).
static const char *const kSplashTitleGlyphs[] = {
    "A", "I",
    "\xE8\xB5\x8B", /* 赋 */
    "\xE8\x83\xBD", /* 能 */
    "\xE8\xAE\xBE", /* 设 */
    "\xE8\xAE\xA1", /* 计 */
    "\xEF\xBC\x8C", /* ， */
    "\xE8\xAE\xBE", /* 设 */
    "\xE8\xAE\xA1", /* 计 */
    "\xE7\x82\xB9", /* 点 */
    "\xE4\xBA\xAE", /* 亮 */
    "A", "I",
};
static constexpr int kSplashTitleGlyphCount =
    static_cast<int>(sizeof(kSplashTitleGlyphs) / sizeof(kSplashTitleGlyphs[0]));

static const char *kStepNames[SPLASH_STEP_COUNT] = {
    "Display / LVGL",
    "Audio Service",
    "MCP Tools",
    "Network",
    "WiFi Connected",
    "Assets",
    "Version Check",
    "Protocol",
    "System Ready",
};

struct SplashContext {
    Display *display = nullptr;
    lv_obj_t *overlay = nullptr;
    lv_obj_t *title_label = nullptr;
    char title_buf[64] = {};
    lv_obj_t *step_list = nullptr;
    lv_obj_t *step_rows[SPLASH_STEP_COUNT] = {};
    lv_obj_t *step_icons[SPLASH_STEP_COUNT] = {};
    lv_obj_t *step_labels[SPLASH_STEP_COUNT] = {};
    lv_obj_t *progress_bar = nullptr;
    lv_timer_t *title_timer = nullptr;
    int title_glyph_index = 0;
    laser_splash_state_t states[SPLASH_STEP_COUNT] = {};
    int running_step = -1;
    bool active = false;
    bool finishing = false;
};

static SplashContext g_splash;

static lv_color_t state_color(laser_splash_state_t state)
{
    switch (state) {
    case SPLASH_STATE_RUNNING:
        return CYBER_AMBER;
    case SPLASH_STATE_OK:
        return CYBER_GREEN;
    case SPLASH_STATE_FAIL:
        return CYBER_RED;
    default:
        return CYBER_DIM;
    }
}

static const char *state_icon(laser_splash_state_t state)
{
    switch (state) {
    case SPLASH_STATE_RUNNING:
        return LV_SYMBOL_REFRESH;
    case SPLASH_STATE_OK:
        return LV_SYMBOL_OK;
    case SPLASH_STATE_FAIL:
        return LV_SYMBOL_CLOSE;
    default:
        return LV_SYMBOL_DUMMY;
    }
}

static void splash_refresh_step_row(int step)
{
    if (step < 0 || step >= SPLASH_STEP_COUNT || g_splash.step_icons[step] == nullptr) {
        return;
    }
    laser_splash_state_t st = g_splash.states[step];
    lv_label_set_text(g_splash.step_icons[step], state_icon(st));
    lv_obj_set_style_text_color(g_splash.step_icons[step], state_color(st), LV_PART_MAIN);
    if (st == SPLASH_STATE_RUNNING) {
        lv_obj_set_style_opa(g_splash.step_rows[step], LV_OPA_80, LV_PART_MAIN);
    } else {
        lv_obj_set_style_opa(g_splash.step_rows[step], LV_OPA_COVER, LV_PART_MAIN);
    }
}

static void splash_update_progress(void)
{
    if (g_splash.progress_bar == nullptr) {
        return;
    }
    int done = 0;
    for (int i = 0; i < SPLASH_STEP_COUNT; ++i) {
        if (g_splash.states[i] == SPLASH_STATE_OK) {
            done++;
        }
    }
    lv_bar_set_value(g_splash.progress_bar, done, LV_ANIM_ON);
}

static void splash_scroll_to_focus_step(void)
{
    if (g_splash.step_list == nullptr) {
        return;
    }

    int focus = g_splash.running_step;
    if (focus < 0) {
        for (int i = SPLASH_STEP_COUNT - 1; i >= 0; --i) {
            if (g_splash.states[i] != SPLASH_STATE_WAIT) {
                focus = i;
                break;
            }
        }
    }
    if (focus >= 0 && g_splash.step_rows[focus] != nullptr) {
        lv_obj_scroll_to_view(g_splash.step_rows[focus], LV_ANIM_ON);
    }
}

static void splash_refresh_ui(void)
{
    if (!g_splash.active || g_splash.overlay == nullptr) {
        return;
    }
    for (int i = 0; i < SPLASH_STEP_COUNT; ++i) {
        splash_refresh_step_row(i);
    }
    splash_update_progress();
    splash_scroll_to_focus_step();
}

static void splash_async_refresh(void *user_data)
{
    (void)user_data;
    if (g_splash.display == nullptr) {
        return;
    }
    DisplayLockGuard lock(g_splash.display);
    splash_refresh_ui();
}

static void title_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (g_splash.title_label == nullptr) {
        return;
    }
    if (g_splash.title_glyph_index >= kSplashTitleGlyphCount) {
        if (g_splash.title_timer != nullptr) {
            lv_timer_pause(g_splash.title_timer);
        }
        return;
    }

    const char *glyph = kSplashTitleGlyphs[g_splash.title_glyph_index++];
    const size_t used = strlen(g_splash.title_buf);
    const size_t glen = strlen(glyph);
    const size_t cap = sizeof(g_splash.title_buf) - 1;
    if (used + glen <= cap) {
        memcpy(g_splash.title_buf + used, glyph, glen);
        g_splash.title_buf[used + glen] = '\0';
    }

    lv_label_set_text(g_splash.title_label, g_splash.title_buf);
}

static void draw_grid_lines(lv_obj_t *parent)
{
    for (int y = 0; y < LV_VER_RES; y += 20) {
        lv_obj_t *line = lv_obj_create(parent);
        lv_obj_set_size(line, LV_HOR_RES, 1);
        lv_obj_set_pos(line, 0, y);
        lv_obj_set_style_bg_color(line, CYBER_GRID, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(line, LV_OPA_30, LV_PART_MAIN);
        lv_obj_set_style_border_width(line, 0, LV_PART_MAIN);
        lv_obj_clear_flag(line, LV_OBJ_FLAG_CLICKABLE);
    }
}

static void overlay_opa_anim(void *obj, int32_t v)
{
    lv_obj_set_style_opa(static_cast<lv_obj_t *>(obj), static_cast<lv_opa_t>(v), LV_PART_MAIN);
}

static void splash_fade_ready_cb(lv_anim_t *anim)
{
    (void)anim;
    if (g_splash.overlay != nullptr) {
        lv_obj_del(g_splash.overlay);
        g_splash.overlay = nullptr;
    }
    if (g_splash.title_timer != nullptr) {
        lv_timer_delete(g_splash.title_timer);
        g_splash.title_timer = nullptr;
    }
    g_splash.active = false;
    g_splash.finishing = false;

    if (g_splash.display != nullptr) {
        laser_ui_init(g_splash.display);
    }
}

void laser_ui_splash_start(Display *display)
{
    if (display == nullptr) {
        return;
    }

    memset(&g_splash, 0, sizeof(g_splash));
    g_splash.display = display;
    g_splash.active = true;

    DisplayLockGuard lock(display);
    BoardUiSetChromeVisible(display, false, false);

    lv_obj_t *screen = lv_screen_active();
    g_splash.overlay = lv_obj_create(screen);
    lv_obj_set_size(g_splash.overlay, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(g_splash.overlay, CYBER_BG, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_splash.overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_splash.overlay, 0, LV_PART_MAIN);
    lv_obj_clear_flag(g_splash.overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_foreground(g_splash.overlay);

    draw_grid_lines(g_splash.overlay);

    g_splash.title_buf[0] = '\0';
    g_splash.title_glyph_index = 0;

    g_splash.title_label = lv_label_create(g_splash.overlay);
    lv_label_set_text(g_splash.title_label, "");
    lv_obj_set_width(g_splash.title_label, LV_HOR_RES - 16);
    lv_label_set_long_mode(g_splash.title_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(g_splash.title_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(g_splash.title_label, CYBER_CYAN, LV_PART_MAIN);
    lv_obj_set_style_text_font(g_splash.title_label, &BUILTIN_TEXT_FONT, LV_PART_MAIN);
    lv_obj_align(g_splash.title_label, LV_ALIGN_TOP_MID, 0, 24);

    g_splash.step_list = lv_obj_create(g_splash.overlay);
    lv_obj_set_size(g_splash.step_list, LV_HOR_RES - 24, 180);
    lv_obj_align(g_splash.step_list, LV_ALIGN_TOP_MID, 0, 72);
    lv_obj_set_style_bg_opa(g_splash.step_list, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_splash.step_list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_splash.step_list, 4, LV_PART_MAIN);
    lv_obj_set_flex_flow(g_splash.step_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g_splash.step_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(g_splash.step_list, 4, LV_PART_MAIN);
    lv_obj_add_flag(g_splash.step_list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(g_splash.step_list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(g_splash.step_list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_snap_y(g_splash.step_list, LV_SCROLL_SNAP_NONE);

    for (int i = 0; i < SPLASH_STEP_COUNT; ++i) {
        g_splash.states[i] = SPLASH_STATE_WAIT;
        g_splash.step_rows[i] = lv_obj_create(g_splash.step_list);
        lv_obj_set_size(g_splash.step_rows[i], LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(g_splash.step_rows[i], LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(g_splash.step_rows[i], 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(g_splash.step_rows[i], 2, LV_PART_MAIN);
        lv_obj_set_flex_flow(g_splash.step_rows[i], LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(g_splash.step_rows[i], LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        g_splash.step_icons[i] = lv_label_create(g_splash.step_rows[i]);
        lv_obj_set_width(g_splash.step_icons[i], 20);
        lv_label_set_text(g_splash.step_icons[i], state_icon(SPLASH_STATE_WAIT));

        g_splash.step_labels[i] = lv_label_create(g_splash.step_rows[i]);
        lv_label_set_text(g_splash.step_labels[i], kStepNames[i]);
        lv_obj_set_style_text_color(g_splash.step_labels[i], UI_COLOR_TEXT, LV_PART_MAIN);
        splash_refresh_step_row(i);
    }

    g_splash.progress_bar = lv_bar_create(g_splash.overlay);
    lv_obj_set_size(g_splash.progress_bar, LV_HOR_RES - 48, 8);
    lv_obj_align(g_splash.progress_bar, LV_ALIGN_BOTTOM_MID, 0, -24);
    lv_bar_set_range(g_splash.progress_bar, 0, SPLASH_STEP_COUNT);
    lv_obj_set_style_bg_color(g_splash.progress_bar, UI_COLOR_PANEL, LV_PART_MAIN);
    lv_obj_set_style_bg_color(g_splash.progress_bar, CYBER_CYAN, LV_PART_INDICATOR);
    lv_bar_set_value(g_splash.progress_bar, 0, LV_ANIM_OFF);

    g_splash.title_timer = lv_timer_create(title_timer_cb, 100, nullptr);

    ESP_LOGI(TAG, "Splash started");
}

void laser_ui_splash_set_step(laser_splash_step_id_t step, laser_splash_state_t state)
{
    if (!g_splash.active || step >= SPLASH_STEP_COUNT) {
        return;
    }

    g_splash.states[step] = state;
    if (state == SPLASH_STATE_RUNNING) {
        g_splash.running_step = static_cast<int>(step);
    } else if (g_splash.running_step == static_cast<int>(step)) {
        g_splash.running_step = -1;
    }

    lv_async_call(splash_async_refresh, nullptr);
}

bool laser_ui_splash_is_active(void)
{
    return g_splash.active;
}

void laser_ui_splash_finish(Display *display)
{
    if (display == nullptr || g_splash.finishing) {
        return;
    }
    if (!g_splash.active) {
        laser_ui_init(display);
        return;
    }

    g_splash.finishing = true;
    DisplayLockGuard lock(display);

    if (g_splash.title_timer != nullptr) {
        lv_timer_pause(g_splash.title_timer);
    }

    if (g_splash.overlay == nullptr) {
        g_splash.active = false;
        g_splash.finishing = false;
        laser_ui_init(display);
        return;
    }

    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, g_splash.overlay);
    lv_anim_set_exec_cb(&anim, overlay_opa_anim);
    lv_anim_set_values(&anim, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_duration(&anim, 300);
    lv_anim_set_ready_cb(&anim, splash_fade_ready_cb);
    lv_anim_start(&anim);

    ESP_LOGI(TAG, "Splash finishing");
}
