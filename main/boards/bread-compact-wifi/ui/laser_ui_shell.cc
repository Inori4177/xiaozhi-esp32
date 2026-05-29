#include "laser_ui_shell.h"
#include "laser_ui_layout.h"
#include "laser_ui_widgets.h"
#include "pages/page_print.h"
#include "pages/page_settings.h"
#include "pages/page_pick.h"
#include "pages/page_xiaozhi.h"

#include <font_awesome.h>

static int pointer_x_in_root(const LaserUiShell *shell)
{
    lv_indev_t *indev = lv_indev_active();
    if (indev == nullptr || shell->root == nullptr) {
        return 0;
    }
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    lv_area_t area;
    lv_obj_get_coords(shell->root, &area);
    return p.x - area.x1;
}

static void page_anim_exec(void *obj, int32_t v)
{
    lv_obj_set_x(static_cast<lv_obj_t *>(obj), v);
}

static void page_close_anim_ready(lv_anim_t *anim)
{
    if (anim == nullptr || anim->var == nullptr) {
        return;
    }
    lv_obj_add_flag(static_cast<lv_obj_t *>(anim->var), LV_OBJ_FLAG_HIDDEN);
}

static void shell_raise_nav(LaserUiShell *shell)
{
    if (shell->nav_dock != nullptr) {
        lv_obj_move_foreground(shell->nav_dock);
    }
}

static void shell_set_page_offset(LaserUiShell *shell, LaserPage page, int x_ofs, bool show)
{
    lv_obj_t *page_obj = shell->pages[static_cast<int>(page)];
    if (page_obj == nullptr) {
        return;
    }
    if (show) {
        lv_obj_remove_flag(page_obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(page_obj, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_set_x(page_obj, x_ofs);
}

static void shell_open_page(LaserUiShell *shell, LaserPage page, bool animate)
{
    if (shell == nullptr) {
        return;
    }

    if (shell->page_open && shell->current == page) {
        shell_raise_nav(shell);
        return;
    }

    if (shell->page_open && shell->current == LaserPage::Pick && page != LaserPage::Pick) {
        page_pick_on_hide();
    }
    if (shell->page_open && shell->current == LaserPage::Print && page != LaserPage::Print) {
        page_print_on_hide();
    }

    const int panel_w = lv_obj_get_width(shell->content_host);
    for (int i = 0; i < static_cast<int>(LaserPage::Count); ++i) {
        if (shell->pages[i] == nullptr) {
            continue;
        }
        if (i == static_cast<int>(page)) {
            continue;
        }
        lv_obj_add_flag(shell->pages[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_x(shell->pages[i], -panel_w);
    }

    shell->current = page;
    shell->page_open = true;

    lv_obj_t *target = shell->pages[static_cast<int>(page)];
    if (target == nullptr) {
        return;
    }

    lv_obj_remove_flag(target, LV_OBJ_FLAG_HIDDEN);

    for (int i = 0; i < static_cast<int>(LaserPage::Count); ++i) {
        if (shell->nav_btns[i] == nullptr) {
            continue;
        }
        if (i == static_cast<int>(page)) {
            lv_obj_add_state(shell->nav_btns[i], LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(shell->nav_btns[i], LV_STATE_CHECKED);
        }
    }

    if (page == LaserPage::Pick) {
        page_pick_on_show();
    }
    if (page == LaserPage::Print) {
        page_print_on_show();
    }

    if (!animate) {
        lv_obj_set_x(target, 0);
        shell_raise_nav(shell);
        return;
    }

    lv_anim_del(target, page_anim_exec);
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, target);
    lv_anim_set_exec_cb(&anim, page_anim_exec);
    lv_anim_set_values(&anim, -panel_w, 0);
    lv_anim_set_duration(&anim, UI_PAGE_OPEN_ANIM_MS);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
    lv_anim_start(&anim);
    shell_raise_nav(shell);
}

static void shell_close_page(LaserUiShell *shell, bool animate)
{
    if (shell == nullptr || !shell->page_open) {
        return;
    }

    if (shell->current == LaserPage::Pick) {
        page_pick_on_hide();
    }
    if (shell->current == LaserPage::Print) {
        page_print_on_hide();
    }

    const int panel_w = lv_obj_get_width(shell->content_host);
    lv_obj_t *target = shell->pages[static_cast<int>(shell->current)];
    if (target == nullptr) {
        shell->page_open = false;
        for (int i = 0; i < static_cast<int>(LaserPage::Count); ++i) {
            if (shell->nav_btns[i] != nullptr) {
                lv_obj_clear_state(shell->nav_btns[i], LV_STATE_CHECKED);
            }
        }
        return;
    }

    if (!animate) {
        for (int i = 0; i < static_cast<int>(LaserPage::Count); ++i) {
            shell_set_page_offset(shell, static_cast<LaserPage>(i), -panel_w, false);
        }
        shell->page_open = false;
        for (int i = 0; i < static_cast<int>(LaserPage::Count); ++i) {
            if (shell->nav_btns[i] != nullptr) {
                lv_obj_clear_state(shell->nav_btns[i], LV_STATE_CHECKED);
            }
        }
        shell_raise_nav(shell);
        return;
    }

    lv_anim_del(target, page_anim_exec);
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, target);
    lv_anim_set_exec_cb(&anim, page_anim_exec);
    lv_anim_set_values(&anim, lv_obj_get_x(target), -panel_w);
    lv_anim_set_duration(&anim, UI_PAGE_CLOSE_ANIM_MS);
    lv_anim_set_path_cb(&anim, lv_anim_path_linear);
    lv_anim_set_completed_cb(&anim, page_close_anim_ready);
    lv_anim_start(&anim);

    shell->page_open = false;
    for (int i = 0; i < static_cast<int>(LaserPage::Count); ++i) {
        if (shell->nav_btns[i] != nullptr) {
            lv_obj_clear_state(shell->nav_btns[i], LV_STATE_CHECKED);
        }
    }
    shell_raise_nav(shell);
}

static int clamp_int(int v, int lo, int hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

static void nav_btn_reset_translate(lv_obj_t *btn)
{
    lv_obj_set_style_translate_x(btn, 0, LV_PART_MAIN);
}

static void nav_dock_drag_cb(lv_event_t *e)
{
    auto *shell = static_cast<LaserUiShell *>(lv_event_get_user_data(e));
    lv_obj_t *btn = static_cast<lv_obj_t *>(lv_event_get_target(e));
    const int page_i = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(btn)));
    const lv_event_code_t code = lv_event_get_code(e);
    const bool is_active_page = shell->page_open && static_cast<int>(shell->current) == page_i;

    if (code == LV_EVENT_PRESSED) {
        shell->nav_drag_origin_x = pointer_x_in_root(shell);
        shell->nav_drag_page = page_i;
        shell->nav_drag_snapped = false;
        shell->nav_suppress_click = false;
        nav_btn_reset_translate(btn);
        return;
    }

    if (code == LV_EVENT_PRESSING) {
        const int dx = pointer_x_in_root(shell) - shell->nav_drag_origin_x;

        if (!shell->nav_drag_snapped) {
            int follow = 0;
            if (!is_active_page) {
                follow = clamp_int(dx, 0, UI_NAV_DRAG_FOLLOW_MAX);
            } else {
                follow = clamp_int(dx, -UI_NAV_DRAG_FOLLOW_MAX, 0);
            }
            lv_obj_set_style_translate_x(btn, follow, LV_PART_MAIN);

            if (dx >= UI_NAV_DRAG_OPEN_PX && !is_active_page) {
                shell_open_page(shell, static_cast<LaserPage>(page_i), true);
                shell->nav_drag_snapped = true;
                shell->nav_suppress_click = true;
                nav_btn_reset_translate(btn);
            } else if (dx <= -static_cast<int>(UI_NAV_DRAG_CLOSE_PX) && is_active_page) {
                shell_close_page(shell, true);
                shell->nav_drag_snapped = true;
                shell->nav_suppress_click = true;
                nav_btn_reset_translate(btn);
            }
        }
        return;
    }

    if (code == LV_EVENT_RELEASED) {
        if (!shell->nav_drag_snapped) {
            const int dx = pointer_x_in_root(shell) - shell->nav_drag_origin_x;
            if (dx >= UI_NAV_DRAG_RELEASE_OPEN_PX && !is_active_page) {
                shell_open_page(shell, static_cast<LaserPage>(page_i), true);
                shell->nav_suppress_click = true;
            } else if (dx <= -static_cast<int>(UI_NAV_DRAG_RELEASE_CLOSE_PX) && is_active_page) {
                shell_close_page(shell, true);
                shell->nav_suppress_click = true;
            } else if (dx > 4 || dx < -4) {
                shell->nav_suppress_click = true;
            }
        }
        nav_btn_reset_translate(btn);
        shell->nav_drag_page = -1;
        shell->nav_drag_snapped = false;
    }
}

static void nav_dock_click_cb(lv_event_t *e)
{
    auto *shell = static_cast<LaserUiShell *>(lv_event_get_user_data(e));
    if (shell->nav_suppress_click) {
        shell->nav_suppress_click = false;
        return;
    }
    lv_obj_t *btn = static_cast<lv_obj_t *>(lv_event_get_target(e));
    const int page_i = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(btn)));
    shell_open_page(shell, static_cast<LaserPage>(page_i), true);
}

void laser_ui_shell_switch_page(LaserUiShell *shell, LaserPage page)
{
    shell_open_page(shell, page, false);
}

void laser_ui_shell_init(LaserUiShell *shell, lv_obj_t *screen)
{
    if (shell == nullptr || screen == nullptr) {
        return;
    }

    laser_ui_apply_main_gradient(screen);

    shell->root = lv_obj_create(screen);
    lv_obj_set_size(shell->root, LV_HOR_RES, UI_MAIN_H);
    lv_obj_align(shell->root, LV_ALIGN_TOP_MID, 0, UI_TOP_H);
    lv_obj_set_style_bg_opa(shell->root, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(shell->root, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(shell->root, 0, LV_PART_MAIN);
    lv_obj_clear_flag(shell->root, LV_OBJ_FLAG_SCROLLABLE);

    shell->content_host = lv_obj_create(shell->root);
    lv_obj_set_size(shell->content_host, UI_CONTENT_W, LV_PCT(100));
    lv_obj_set_pos(shell->content_host, UI_NAV_DOCK_W, 0);
    lv_obj_set_style_bg_opa(shell->content_host, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(shell->content_host, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(shell->content_host, 0, LV_PART_MAIN);
    lv_obj_add_flag(shell->content_host, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_clear_flag(shell->content_host, LV_OBJ_FLAG_SCROLLABLE);

    shell->pages[static_cast<int>(LaserPage::Print)] = page_print_create(shell->content_host);
    shell->pages[static_cast<int>(LaserPage::Settings)] = page_settings_create(shell->content_host);
    shell->pages[static_cast<int>(LaserPage::Pick)] = page_pick_create(shell->content_host);
    shell->pages[static_cast<int>(LaserPage::Xiaozhi)] = page_xiaozhi_create(shell->content_host);

    for (int i = 0; i < static_cast<int>(LaserPage::Count); ++i) {
        if (shell->pages[i] != nullptr) {
            lv_obj_set_size(shell->pages[i], UI_CONTENT_W, LV_PCT(100));
            lv_obj_add_flag(shell->pages[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    shell->nav_dock = lv_obj_create(shell->root);
    lv_obj_set_size(shell->nav_dock, UI_NAV_DOCK_W, LV_PCT(100));
    lv_obj_set_pos(shell->nav_dock, 0, 0);
    lv_obj_set_style_bg_opa(shell->nav_dock, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(shell->nav_dock, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(shell->nav_dock, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(shell->nav_dock, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(shell->nav_dock, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(shell->nav_dock, 14, LV_PART_MAIN);
    lv_obj_clear_flag(shell->nav_dock, LV_OBJ_FLAG_SCROLLABLE);

    struct NavItem {
        const char *icon;
    };
    const NavItem nav_items[] = {
        {LV_SYMBOL_PLAY},
        {LV_SYMBOL_SETTINGS},
        {LV_SYMBOL_EDIT},
        {FONT_AWESOME_MICROCHIP_AI},
    };

    for (int i = 0; i < static_cast<int>(LaserPage::Count); ++i) {
        shell->nav_btns[i] = laser_ui_create_nav_dock_button(shell->nav_dock, nav_items[i].icon);
        lv_obj_set_user_data(shell->nav_btns[i], reinterpret_cast<void *>(static_cast<intptr_t>(i)));
        lv_obj_add_event_cb(shell->nav_btns[i], nav_dock_drag_cb, LV_EVENT_PRESSED, shell);
        lv_obj_add_event_cb(shell->nav_btns[i], nav_dock_drag_cb, LV_EVENT_PRESSING, shell);
        lv_obj_add_event_cb(shell->nav_btns[i], nav_dock_drag_cb, LV_EVENT_RELEASED, shell);
        lv_obj_add_event_cb(shell->nav_btns[i], nav_dock_click_cb, LV_EVENT_CLICKED, shell);
    }

    shell_raise_nav(shell);
    shell_open_page(shell, LaserPage::Print, false);
}
