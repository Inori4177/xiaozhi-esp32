#include "laser_ui_splash.h"
#include "laser_ui_layout.h"
#include "laser_ui.h"

#include "assets.h"
#include "display.h"
#include "lvgl_image.h"
#include "boards/common/board_custom_ui.h"

#include <esp_log.h>
#include <font_awesome.h>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

static const char *TAG = "laser_ui_splash";

LV_FONT_DECLARE(BUILTIN_TEXT_FONT);
LV_FONT_DECLARE(BUILTIN_ICON_FONT);

static const char *const kSplashSlogan =
    "AI"
    "\xE8\xB5\x8B"  /* 赋 */
    "\xE8\x83\xBD"  /* 能 */
    "\xE8\xAE\xBE"  /* 设 */
    "\xE8\xAE\xA1"  /* 计 */
    "\xEF\xBC\x8C"  /* ， */
    "\xE8\xAE\xBE"  /* 设 */
    "\xE8\xAE\xA1"  /* 计 */
    "\xE7\x82\xB9"  /* 点 */
    "\xE4\xBA\xAE"  /* 亮 */
    "AI";

static constexpr int kSplashContentGap = 1;
static constexpr uint32_t kSplashInitFrame1To5HoldMs = 125;
static constexpr uint32_t kSplashInitFrame6To9HoldMs = 125;
static constexpr uint32_t kSplashInitFrame10To12HoldMs = 160;   
static constexpr uint32_t kSplashInitFrame13To17HoldMs = 125;
static constexpr uint32_t kSplashInitFrame18To24HoldMs = 125;
static constexpr uint32_t kSplashInitFrame25To27HoldMs =125;
static constexpr uint32_t kSplashSloganTickMs = 212;
static constexpr uint32_t kSplashSloganFadeDelayMs = 0;
static constexpr uint32_t kSplashSloganFadeMs = 2200;
static constexpr uint32_t kSplashSloganHoldMs = 500;
static constexpr int kSplashInitW = 217;
static constexpr int kSplashInitH = 180;
static constexpr unsigned kSplashInitFrameCount = 27;

enum SplashStage {
    SPLASH_STAGE_INIT = 0,
    SPLASH_STAGE_SLOGAN,
};

struct SplashContext {
    Display *display = nullptr;
    lv_obj_t *overlay = nullptr;
    lv_obj_t *init_image = nullptr;
    lv_obj_t *content_row = nullptr;
    lv_obj_t *title_icon = nullptr;
    lv_obj_t *title_label = nullptr;
    lv_timer_t *stage_timer = nullptr;
    std::vector<std::shared_ptr<LvglBorrowedImage>> init_frames;
    unsigned init_frame_idx = 0;
    uint32_t slogan_elapsed_ms = 0;
    SplashStage stage = SPLASH_STAGE_INIT;
    bool pending_finish = false;
    bool active = false;
    bool finishing = false;
};

static SplashContext g_splash;

static uint32_t splash_init_frame_hold_ms(unsigned frame_idx)
{
    const unsigned frame_no = frame_idx + 1U;
    if (frame_no <= 5U) {
        return kSplashInitFrame1To5HoldMs;
    }
    if (frame_no <= 9U) {
        return kSplashInitFrame6To9HoldMs;
    }
    if (frame_no <= 12U) {
        return kSplashInitFrame10To12HoldMs;
    }
    if (frame_no <= 17U) {
        return kSplashInitFrame13To17HoldMs;
    }
    if (frame_no <= 24U) {
        return kSplashInitFrame18To24HoldMs;
    }
    return kSplashInitFrame25To27HoldMs;
}

static void splash_set_stage_timer_period(lv_timer_t *timer, uint32_t period_ms)
{
    if (timer != nullptr) {
        lv_timer_set_period(timer, period_ms);
        lv_timer_reset(timer);
    }
}

static bool splash_load_init_frames(void)
{
    auto &assets = Assets::GetInstance();
    g_splash.init_frames.clear();
    g_splash.init_frames.reserve(kSplashInitFrameCount);

    for (unsigned i = 1; i <= kSplashInitFrameCount; ++i) {
        char name[16];
        snprintf(name, sizeof(name), "init_%u.bin", i);

        void *ptr = nullptr;
        size_t size = 0;
        if (!assets.GetAssetData(name, ptr, size)) {
            ESP_LOGW(TAG, "Init splash asset missing: %s", name);
            g_splash.init_frames.clear();
            return false;
        }
        ESP_LOGI(TAG, "Init splash asset loaded: %s ptr=%p size=%u", name, ptr, (unsigned)size);

        try {
            g_splash.init_frames.push_back(std::make_shared<LvglBorrowedImage>(ptr, size));
        } catch (...) {
            ESP_LOGE(TAG, "Failed to decode init splash asset: %s", name);
            g_splash.init_frames.clear();
            return false;
        }
    }

    return !g_splash.init_frames.empty();
}

static void overlay_opa_anim(void *obj, int32_t v)
{
    lv_obj_set_style_opa(static_cast<lv_obj_t *>(obj), static_cast<lv_opa_t>(v), LV_PART_MAIN);
}

static void label_opa_anim(void *obj, int32_t v)
{
    lv_obj_set_style_text_opa(static_cast<lv_obj_t *>(obj), static_cast<lv_opa_t>(v), LV_PART_MAIN);
}

static void splash_deferred_laser_ui_init(void *user_data)
{
    auto *display = static_cast<Display *>(user_data);
    if (display != nullptr) {
        laser_ui_init(display);
    }
}

static void splash_fade_ready_cb(lv_anim_t *anim)
{
    (void)anim;
    if (g_splash.stage_timer != nullptr) {
        lv_timer_delete(g_splash.stage_timer);
        g_splash.stage_timer = nullptr;
    }
    if (g_splash.overlay != nullptr) {
        lv_obj_del(g_splash.overlay);
        g_splash.overlay = nullptr;
    }
    g_splash.active = false;
    g_splash.finishing = false;
    g_splash.pending_finish = false;

    if (g_splash.display != nullptr) {
        lv_async_call(splash_deferred_laser_ui_init, g_splash.display);
    }
}

static void splash_begin_finish(void)
{
    if (!g_splash.active || g_splash.finishing || g_splash.overlay == nullptr) {
        return;
    }

    g_splash.finishing = true;
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

static void splash_show_slogan_stage(void)
{
    ESP_LOGI(TAG, "Splash switching to slogan stage");
    g_splash.stage = SPLASH_STAGE_SLOGAN;
    g_splash.slogan_elapsed_ms = 0;

    if (g_splash.init_image != nullptr) {
        lv_obj_del(g_splash.init_image);
        g_splash.init_image = nullptr;
    }

    if (g_splash.overlay == nullptr) {
        return;
    }

    lv_obj_set_style_bg_color(g_splash.overlay, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(g_splash.overlay, LV_OPA_COVER, LV_PART_MAIN);

    g_splash.content_row = lv_obj_create(g_splash.overlay);
    lv_obj_set_size(g_splash.content_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(g_splash.content_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_splash.content_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_splash.content_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(g_splash.content_row, kSplashContentGap, LV_PART_MAIN);
    lv_obj_set_flex_flow(g_splash.content_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(g_splash.content_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_center(g_splash.content_row);

    g_splash.title_icon = lv_label_create(g_splash.content_row);
    lv_label_set_text(g_splash.title_icon, FONT_AWESOME_MICROCHIP_AI);
    lv_obj_set_style_text_font(g_splash.title_icon, &BUILTIN_ICON_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(g_splash.title_icon, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_text_opa(g_splash.title_icon, LV_OPA_TRANSP, LV_PART_MAIN);

    g_splash.title_label = lv_label_create(g_splash.content_row);
    lv_label_set_text(g_splash.title_label, kSplashSlogan);
    lv_label_set_long_mode(g_splash.title_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(g_splash.title_label, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(g_splash.title_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_text_color(g_splash.title_label, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_text_font(g_splash.title_label, &BUILTIN_TEXT_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_opa(g_splash.title_label, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_anim_t icon_anim;
    lv_anim_init(&icon_anim);
    lv_anim_set_var(&icon_anim, g_splash.title_icon);
    lv_anim_set_exec_cb(&icon_anim, label_opa_anim);
    lv_anim_set_values(&icon_anim, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_delay(&icon_anim, kSplashSloganFadeDelayMs);
    lv_anim_set_duration(&icon_anim, kSplashSloganFadeMs);
    lv_anim_set_path_cb(&icon_anim, lv_anim_path_ease_in_out);
    lv_anim_start(&icon_anim);

    lv_anim_t title_anim;
    lv_anim_init(&title_anim);
    lv_anim_set_var(&title_anim, g_splash.title_label);
    lv_anim_set_exec_cb(&title_anim, label_opa_anim);
    lv_anim_set_values(&title_anim, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_delay(&title_anim, kSplashSloganFadeDelayMs);
    lv_anim_set_duration(&title_anim, kSplashSloganFadeMs);
    lv_anim_set_path_cb(&title_anim, lv_anim_path_ease_in_out);
    lv_anim_start(&title_anim);
}

static void splash_stage_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (!g_splash.active || g_splash.finishing) {
        return;
    }

    if (g_splash.stage == SPLASH_STAGE_INIT) {
        if (g_splash.init_frame_idx + 1U < g_splash.init_frames.size()) {
            ++g_splash.init_frame_idx;
            ESP_LOGI(TAG, "Splash show init frame %u/%u hold=%ums", g_splash.init_frame_idx + 1U,
                     (unsigned)g_splash.init_frames.size(),
                     (unsigned)splash_init_frame_hold_ms(g_splash.init_frame_idx));
            lv_image_set_src(g_splash.init_image, g_splash.init_frames[g_splash.init_frame_idx]->image_dsc());
            splash_set_stage_timer_period(timer, splash_init_frame_hold_ms(g_splash.init_frame_idx));
            return;
        }
        splash_show_slogan_stage();
        splash_set_stage_timer_period(timer, kSplashSloganTickMs);
        return;
    }

    g_splash.slogan_elapsed_ms += kSplashSloganTickMs;
    if (g_splash.pending_finish &&
        g_splash.slogan_elapsed_ms >= kSplashSloganFadeDelayMs + kSplashSloganFadeMs + kSplashSloganHoldMs) {
        splash_begin_finish();
    }
}

void laser_ui_splash_start(Display *display)
{
    if (display == nullptr) {
        return;
    }

    g_splash = SplashContext{};
    g_splash.display = display;
    g_splash.active = true;
    g_splash.stage = SPLASH_STAGE_INIT;

    DisplayLockGuard lock(display);
    BoardUiSetChromeVisible(display, false, false);

    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    g_splash.overlay = lv_obj_create(screen);
    lv_obj_set_size(g_splash.overlay, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(g_splash.overlay, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(g_splash.overlay, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_splash.overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_splash.overlay, 0, LV_PART_MAIN);
    lv_obj_clear_flag(g_splash.overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_foreground(g_splash.overlay);

    if (splash_load_init_frames()) {
        g_splash.init_image = lv_image_create(g_splash.overlay);
        lv_image_set_src(g_splash.init_image, g_splash.init_frames[0]->image_dsc());
        ESP_LOGI(TAG, "Splash show init frame 1/%u", (unsigned)g_splash.init_frames.size());
        lv_obj_set_size(g_splash.init_image, kSplashInitW, kSplashInitH);
        lv_obj_center(g_splash.init_image);
        lv_obj_set_style_bg_opa(g_splash.init_image, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_clear_flag(g_splash.init_image, LV_OBJ_FLAG_CLICKABLE);
    } else {
        ESP_LOGW(TAG, "Splash init frames unavailable, skip to slogan");
        splash_show_slogan_stage();
    }

    const uint32_t first_hold_ms =
        g_splash.stage == SPLASH_STAGE_INIT ? splash_init_frame_hold_ms(0) : kSplashSloganTickMs;
    g_splash.stage_timer = lv_timer_create(splash_stage_timer_cb, first_hold_ms, nullptr);

    ESP_LOGI(TAG, "Splash started");
}

void laser_ui_splash_set_step(laser_splash_step_id_t step, laser_splash_state_t state)
{
    (void)step;
    (void)state;
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

    g_splash.pending_finish = true;
    ESP_LOGI(TAG, "Splash finish requested at stage=%d elapsed=%u", (int)g_splash.stage,
             (unsigned)g_splash.slogan_elapsed_ms);
    if (g_splash.stage == SPLASH_STAGE_SLOGAN) {
        DisplayLockGuard lock(display);
        if (g_splash.slogan_elapsed_ms >= kSplashSloganFadeDelayMs + kSplashSloganFadeMs + kSplashSloganHoldMs) {
            splash_begin_finish();
        }
    }
}
