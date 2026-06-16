#include "laser_ui.h"
#include "laser_ui_shell.h"
#include "laser_ui_xiaozhi_presenter.h"
#include "laser_ui_events.h"
#include "cnc/ui_cnc_print_service.h"
#include "cnc/ui_cnc_print_status_service.h"
#include "pick/ui_pick_service.h"

#include "display.h"
#include "boards/common/board_custom_ui.h"

#include <esp_log.h>
#include <esp_heap_caps.h>

static const char *TAG = "laser_ui";

static LaserUiShell g_shell;
static bool g_initialized = false;

void laser_ui_init(Display *display)
{
    if (display == nullptr || g_initialized) {
        return;
    }

    DisplayLockGuard lock(display);
    BoardUiSetChromeVisible(display, true, false);

    laser_ui_events_init();
    ui_cnc_print_service_init();
    ui_cnc_print_status_service_init();
    ui_pick_service_init();

    lv_obj_t *screen = lv_screen_active();
    laser_ui_shell_init(&g_shell, screen);
    if (g_shell.root != nullptr) {
        lv_obj_move_foreground(g_shell.root);
    }
    laser_ui_xiaozhi_presenter_raise_overlay();

    g_initialized = true;
    ESP_LOGI(TAG, "Laser UI ready (480x320), internal free %u min %u",
             static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
             static_cast<unsigned>(heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL)));
}
