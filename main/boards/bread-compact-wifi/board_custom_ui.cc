/**
 * MSP3525 laser UI integration (touch + WebUI interaction chip).
 */
#include <sdkconfig.h>

#if CONFIG_MSP3525_LASER_UI

#include "boards/common/board_custom_ui.h"

#include "display.h"
#include "lcd_display.h"
#include "lvgl_theme.h"

#include "laser_ui_splash.h"

static void msp3525_on_display_init(Display *display)
{
    if (display == nullptr) {
        return;
    }
    display->SetupUI();
    display->SetTheme(LvglThemeManager::GetInstance().GetTheme("dark"));
    laser_ui_splash_start(display);
    laser_ui_splash_set_step(SPLASH_STEP_DISPLAY, SPLASH_STATE_OK);
}

static void msp3525_on_splash_step(BoardUiSplashStep step, BoardUiSplashState state)
{
    laser_ui_splash_set_step(static_cast<laser_splash_step_id_t>(step),
                             static_cast<laser_splash_state_t>(state));
}

static void msp3525_on_activation_done(Display *display)
{
    laser_ui_splash_set_step(SPLASH_STEP_READY, SPLASH_STATE_OK);
    laser_ui_splash_finish(display);
}

static void msp3525_on_chat_message(const char *role, const char *content)
{
    (void)role;
    (void)content;
}

static void msp3525_on_chrome_visible(Display *display, bool top_bottom_visible, bool center_visible)
{
    auto *lcd = dynamic_cast<LcdDisplay *>(display);
    if (lcd != nullptr) {
        lcd->SetOverlayChromeVisible(top_bottom_visible, center_visible);
    }
}

static const BoardCustomUiOps kMsp3525LaserUiOps = {
    .on_display_init = msp3525_on_display_init,
    .on_splash_step = msp3525_on_splash_step,
    .on_activation_done = msp3525_on_activation_done,
    .on_chat_message = msp3525_on_chat_message,
    .on_chrome_visible = msp3525_on_chrome_visible,
};

const BoardCustomUiOps *Msp3525GetLaserUiOps(void)
{
    return &kMsp3525LaserUiOps;
}

#endif /* CONFIG_MSP3525_LASER_UI */
