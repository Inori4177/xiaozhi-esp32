#include "laser_ui_shell.h"
#include "laser_ui_layout.h"
#include "laser_ui_widgets.h"
#include "pages/page_print.h"
#include "pages/page_settings.h"
#include "pages/page_design.h"
#include "pages/page_xiaozhi.h"

#include <font_awesome.h>

static void nav_click_cb(lv_event_t *e)
{
    auto *shell = static_cast<LaserUiShell *>(lv_event_get_user_data(e));
    auto page = static_cast<LaserPage>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(static_cast<lv_obj_t *>(lv_event_get_target(e)))));
    laser_ui_shell_switch_page(shell, page);
}

void laser_ui_shell_switch_page(LaserUiShell *shell, LaserPage page)
{
    if (shell == nullptr) {
        return;
    }
    shell->current = page;
    for (int i = 0; i < static_cast<int>(LaserPage::Count); ++i) {
        if (shell->pages[i] != nullptr) {
            if (i == static_cast<int>(page)) {
                lv_obj_remove_flag(shell->pages[i], LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(shell->pages[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        if (shell->nav_btns[i] != nullptr) {
            if (i == static_cast<int>(page)) {
                lv_obj_add_state(shell->nav_btns[i], LV_STATE_CHECKED);
            } else {
                lv_obj_clear_state(shell->nav_btns[i], LV_STATE_CHECKED);
            }
        }
    }
}

void laser_ui_shell_init(LaserUiShell *shell, lv_obj_t *screen)
{
    if (shell == nullptr || screen == nullptr) {
        return;
    }

    shell->root = lv_obj_create(screen);
    lv_obj_set_size(shell->root, LV_HOR_RES, UI_MAIN_H);
    lv_obj_align(shell->root, LV_ALIGN_TOP_MID, 0, UI_TOP_H);
    lv_obj_set_style_bg_color(shell->root, UI_COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_border_width(shell->root, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(shell->root, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(shell->root, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(shell->root, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(shell->root, LV_OBJ_FLAG_SCROLLABLE);

    shell->nav_sidebar = lv_obj_create(shell->root);
    lv_obj_set_size(shell->nav_sidebar, UI_NAV_W, LV_PCT(100));
    lv_obj_set_style_bg_color(shell->nav_sidebar, UI_COLOR_NAV, LV_PART_MAIN);
    lv_obj_set_style_border_width(shell->nav_sidebar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(shell->nav_sidebar, 4, LV_PART_MAIN);
    lv_obj_set_flex_flow(shell->nav_sidebar, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(shell->nav_sidebar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(shell->nav_sidebar, 4, LV_PART_MAIN);

    struct NavItem {
        const char *icon;
        const char *label;
    };
    const NavItem nav_items[] = {
        {LV_SYMBOL_PLAY, "打印"},
        {LV_SYMBOL_SETTINGS, "设置"},
        {LV_SYMBOL_EDIT, "设计"},
        {FONT_AWESOME_MICROCHIP_AI, "小智"},
    };

    for (int i = 0; i < static_cast<int>(LaserPage::Count); ++i) {
        shell->nav_btns[i] = laser_ui_create_nav_button(shell->nav_sidebar, nav_items[i].icon, nav_items[i].label);
        lv_obj_set_user_data(shell->nav_btns[i], reinterpret_cast<void *>(static_cast<intptr_t>(i)));
        lv_obj_add_event_cb(shell->nav_btns[i], nav_click_cb, LV_EVENT_CLICKED, shell);
    }

    shell->content_host = lv_obj_create(shell->root);
    lv_obj_set_size(shell->content_host, UI_CONTENT_W, LV_PCT(100));
    lv_obj_set_style_bg_opa(shell->content_host, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(shell->content_host, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(shell->content_host, 0, LV_PART_MAIN);

    shell->pages[static_cast<int>(LaserPage::Print)] = page_print_create(shell->content_host);
    shell->pages[static_cast<int>(LaserPage::Settings)] = page_settings_create(shell->content_host);
    shell->pages[static_cast<int>(LaserPage::Design)] = page_design_create(shell->content_host);
    shell->pages[static_cast<int>(LaserPage::Xiaozhi)] = page_xiaozhi_create(shell->content_host);

    laser_ui_shell_switch_page(shell, LaserPage::Print);
}
