#include "laser_ui.h"
#include "laser_controller.h"
#include "laser_ui_shell.h"
#include "laser_ui_events.h"
#include "laser_ui_log.h"
#include "pages/page_xiaozhi.h"
#include "pick/ui_pick_service.h"

#include "display.h"
#include "boards/common/board_custom_ui.h"

#include <esp_log.h>

static const char *TAG = "laser_ui";

static LaserUiShell g_shell;
static bool g_initialized = false;

static void log_refresh_async(void *user_data)
{
    (void)user_data;
    page_xiaozhi_refresh_log();
}

static void log_refresh_cb(void)
{
    lv_async_call(log_refresh_async, nullptr);
}

void laser_ui_init(Display *display)
{
    if (display == nullptr || g_initialized) {
        return;
    }

    DisplayLockGuard lock(display);
    BoardUiSetChromeVisible(display, true, false);

    laser_ui_events_init();
    ui_pick_service_init();
    laser_controller_init();
    laser_ui_log_init();
    laser_ui_log_set_refresh_cb(log_refresh_cb);

    lv_obj_t *screen = lv_screen_active();
    laser_ui_shell_init(&g_shell, screen);
    if (g_shell.root != nullptr) {
        lv_obj_move_foreground(g_shell.root);
    }

    g_initialized = true;
    ESP_LOGI(TAG, "Laser UI ready (480x320)");
}
