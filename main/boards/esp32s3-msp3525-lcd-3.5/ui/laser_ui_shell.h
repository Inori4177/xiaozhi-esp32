#pragma once

#include <lvgl.h>

enum class LaserPage {
    Print = 0,
    Settings,
    Design,
    Xiaozhi,
    Count,
};

struct LaserUiShell {
    lv_obj_t *root = nullptr;
    lv_obj_t *nav_sidebar = nullptr;
    lv_obj_t *content_host = nullptr;
    lv_obj_t *nav_btns[4] = {};
    lv_obj_t *pages[4] = {};
    LaserPage current = LaserPage::Print;
};

void laser_ui_shell_init(LaserUiShell *shell, lv_obj_t *screen);
void laser_ui_shell_switch_page(LaserUiShell *shell, LaserPage page);
