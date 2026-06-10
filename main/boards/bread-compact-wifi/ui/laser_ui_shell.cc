#include "laser_ui_shell.h"

#include "laser_ui_layout.h"
#include "laser_ui_widgets.h"
#include "assets/laser_ui_images.h"
#include "cnc/ui_cnc_print_service.h"
#include "pages/page_pick.h"
#include "pages/page_print.h"
#include "pages/page_settings.h"
#include "pages/page_voice_ai.h"

#include <cstdint>

namespace {

ui_cnc_work_state_t g_shell_last_cnc_state = UI_CNC_WORK_IDLE;
lv_timer_t *g_cnc_poll_timer = nullptr;

const lv_image_dsc_t *nav_image_for_page(LaserPage page)
{
    switch (page) {
    case LaserPage::Print:
        return &printpage;
    case LaserPage::Settings:
        return &settingpage;
    case LaserPage::Pick:
        return &pickpage;
    case LaserPage::VoiceAi:
        return &xiaozhipage;
    case LaserPage::Count:
        break;
    }
    return &printpage;
}

void shell_raise_nav_layer(LaserUiShell *shell)
{
    if (shell != nullptr && shell->nav_layer != nullptr) {
        lv_obj_move_foreground(shell->nav_layer);
    }
}

void page_anim_exec(void *obj, int32_t v)
{
    lv_obj_set_x(static_cast<lv_obj_t *>(obj), v);
}

void page_close_anim_ready(lv_anim_t *anim)
{
    if (anim != nullptr && anim->var != nullptr) {
        lv_obj_add_flag(static_cast<lv_obj_t *>(anim->var), LV_OBJ_FLAG_HIDDEN);
    }
}

void shell_set_page_offset(LaserUiShell *shell, LaserPage page, int x_ofs, bool show)
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

void shell_set_nav_state(LaserUiShell *shell, LaserPage page)
{
    if (shell == nullptr || shell->nav_image == nullptr) {
        return;
    }
    lv_image_set_src(shell->nav_image, nav_image_for_page(page));
}

void shell_notify_page_hide(LaserPage page)
{
    if (page == LaserPage::Pick) {
        page_pick_on_hide();
    }
    if (page == LaserPage::Print) {
        page_print_on_hide();
    }
    if (page == LaserPage::VoiceAi) {
        page_voice_ai_on_hide();
    }
}

void shell_notify_page_show(LaserPage page)
{
    if (page == LaserPage::Pick) {
        page_pick_on_show();
    }
    if (page == LaserPage::Print) {
        page_print_on_show();
    }
    if (page == LaserPage::VoiceAi) {
        page_voice_ai_on_show();
    }
}

void shell_open_page(LaserUiShell *shell, LaserPage page, bool animate)
{
    if (shell == nullptr) {
        return;
    }

    if (shell->page_open && shell->current == page) {
        shell_set_nav_state(shell, page);
        shell_raise_nav_layer(shell);
        return;
    }

    if (shell->page_open && shell->current != page) {
        shell_notify_page_hide(shell->current);
    }

    const int panel_w = lv_obj_get_width(shell->content_host);
    for (int i = 0; i < static_cast<int>(LaserPage::Count); ++i) {
        if (shell->pages[i] == nullptr || i == static_cast<int>(page)) {
            continue;
        }
        lv_obj_add_flag(shell->pages[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_x(shell->pages[i], -panel_w);
    }

    shell->current = page;
    shell->page_open = true;
    shell_set_nav_state(shell, page);

    lv_obj_t *target = shell->pages[static_cast<int>(page)];
    if (target == nullptr) {
        return;
    }

    lv_obj_remove_flag(target, LV_OBJ_FLAG_HIDDEN);
    shell_notify_page_show(page);

    if (!animate) {
        lv_obj_set_x(target, 0);
        shell_raise_nav_layer(shell);
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
    shell_raise_nav_layer(shell);
}

void nav_hotspot_event_cb(lv_event_t *e)
{
    auto *shell = static_cast<LaserUiShell *>(lv_event_get_user_data(e));
    if (shell == nullptr) {
        return;
    }

    lv_obj_t *hotspot = static_cast<lv_obj_t *>(lv_event_get_target(e));
    const lv_event_code_t code = lv_event_get_code(e);
    intptr_t page_value = reinterpret_cast<intptr_t>(lv_obj_get_user_data(hotspot));
    if (page_value < 0 || page_value >= static_cast<intptr_t>(LaserPage::Count)) {
        return;
    }

    switch (code) {
    case LV_EVENT_PRESSED:
        lv_obj_set_style_bg_color(hotspot, UI_COLOR_AQUA, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(hotspot, LV_OPA_40, LV_PART_MAIN);
        lv_obj_set_style_border_width(hotspot, 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(hotspot, UI_COLOR_ACCENT_GLOW, LV_PART_MAIN);
        lv_obj_set_style_border_opa(hotspot, LV_OPA_70, LV_PART_MAIN);
        break;
    case LV_EVENT_RELEASED:
    case LV_EVENT_PRESS_LOST:
        lv_obj_set_style_bg_opa(hotspot, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(hotspot, 0, LV_PART_MAIN);
        lv_obj_set_style_border_opa(hotspot, LV_OPA_TRANSP, LV_PART_MAIN);
        break;
    case LV_EVENT_CLICKED:
        shell_open_page(shell, static_cast<LaserPage>(page_value), true);
        break;
    default:
        break;
    }
}

lv_obj_t *create_nav_hotspot(LaserUiShell *shell, int x, int y, int w, int h, LaserPage page)
{
    lv_obj_t *hotspot = lv_obj_create(shell->nav_layer);
    lv_obj_set_pos(hotspot, x, y);
    lv_obj_set_size(hotspot, w, h);
    lv_obj_set_style_bg_opa(hotspot, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(hotspot, 0, LV_PART_MAIN);
    lv_obj_set_style_border_opa(hotspot, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_radius(hotspot, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_all(hotspot, 0, LV_PART_MAIN);
    lv_obj_clear_flag(hotspot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(hotspot, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(hotspot, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_set_ext_click_area(hotspot, UI_NAV_HOTSPOT_EXTEND);
    lv_obj_set_user_data(hotspot, reinterpret_cast<void *>(static_cast<intptr_t>(page)));
    lv_obj_add_event_cb(hotspot, nav_hotspot_event_cb, LV_EVENT_PRESSED, shell);
    lv_obj_add_event_cb(hotspot, nav_hotspot_event_cb, LV_EVENT_RELEASED, shell);
    lv_obj_add_event_cb(hotspot, nav_hotspot_event_cb, LV_EVENT_PRESS_LOST, shell);
    lv_obj_add_event_cb(hotspot, nav_hotspot_event_cb, LV_EVENT_CLICKED, shell);
    return hotspot;
}

void cnc_state_poll_cb(lv_timer_t *timer)
{
    auto *shell = static_cast<LaserUiShell *>(lv_timer_get_user_data(timer));
    if (shell == nullptr) {
        return;
    }

    ui_cnc_print_status_t st{};
    ui_cnc_print_service_get_status(&st);

    if (g_shell_last_cnc_state == UI_CNC_WORK_IDLE && st.state == UI_CNC_WORK_RUNNING) {
        shell_open_page(shell, LaserPage::VoiceAi, true);
    }

    g_shell_last_cnc_state = st.state;
}

}  // namespace

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
    laser_ui_add_background_pattern(shell->root);

    shell->content_host = lv_obj_create(shell->root);
    lv_obj_set_size(shell->content_host, UI_CONTENT_W, UI_MAIN_H);
    lv_obj_set_pos(shell->content_host, UI_CONTENT_X, 0);
    lv_obj_set_layout(shell->content_host, LV_LAYOUT_NONE);
    lv_obj_set_flex_grow(shell->content_host, 0);
    lv_obj_set_style_bg_opa(shell->content_host, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(shell->content_host, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(shell->content_host, 0, LV_PART_MAIN);
    lv_obj_add_flag(shell->content_host, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_clear_flag(shell->content_host, LV_OBJ_FLAG_SCROLLABLE);

    shell->pages[static_cast<int>(LaserPage::Print)] = page_print_create(shell->content_host);
    shell->pages[static_cast<int>(LaserPage::Settings)] = page_settings_create(shell->content_host);
    shell->pages[static_cast<int>(LaserPage::Pick)] = page_pick_create(shell->content_host);
    shell->pages[static_cast<int>(LaserPage::VoiceAi)] = page_voice_ai_create(shell->content_host);

    for (int i = 0; i < static_cast<int>(LaserPage::Count); ++i) {
        if (shell->pages[i] == nullptr) {
            continue;
        }
        lv_obj_set_size(shell->pages[i], UI_CONTENT_W, UI_MAIN_H);
        lv_obj_set_layout(shell->pages[i], LV_LAYOUT_NONE);
        lv_obj_set_flex_grow(shell->pages[i], 0);
        lv_obj_set_style_min_width(shell->pages[i], UI_CONTENT_W, LV_PART_MAIN);
        lv_obj_set_style_min_height(shell->pages[i], UI_MAIN_H, LV_PART_MAIN);
        lv_obj_set_style_max_width(shell->pages[i], UI_CONTENT_W, LV_PART_MAIN);
        lv_obj_set_style_max_height(shell->pages[i], UI_MAIN_H, LV_PART_MAIN);
        lv_obj_add_flag(shell->pages[i], LV_OBJ_FLAG_HIDDEN);
    }

    shell->nav_layer = lv_obj_create(shell->root);
    lv_obj_set_size(shell->nav_layer, LV_HOR_RES, UI_MAIN_H);
    lv_obj_set_pos(shell->nav_layer, 0, 0);
    lv_obj_set_style_bg_opa(shell->nav_layer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(shell->nav_layer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(shell->nav_layer, 0, LV_PART_MAIN);
    lv_obj_clear_flag(shell->nav_layer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(shell->nav_layer, LV_OBJ_FLAG_CLICKABLE);

    shell->nav_image = lv_image_create(shell->nav_layer);
    lv_image_set_src(shell->nav_image, nav_image_for_page(LaserPage::Print));
    lv_obj_set_pos(shell->nav_image, UI_NAV_IMAGE_X, UI_NAV_IMAGE_Y);

    shell->nav_hotspots[static_cast<int>(LaserPage::Print)] =
        create_nav_hotspot(shell, UI_NAV_HOTSPOT_X, UI_NAV_HOTSPOT_Y_RUN,
                           UI_NAV_HOTSPOT_W, UI_NAV_HOTSPOT_H, LaserPage::Print);
    shell->nav_hotspots[static_cast<int>(LaserPage::Settings)] =
        create_nav_hotspot(shell, UI_NAV_HOTSPOT_X, UI_NAV_HOTSPOT_Y_SETTINGS,
                           UI_NAV_HOTSPOT_W, UI_NAV_HOTSPOT_H, LaserPage::Settings);
    shell->nav_hotspots[static_cast<int>(LaserPage::Pick)] =
        create_nav_hotspot(shell, UI_NAV_HOTSPOT_X, UI_NAV_HOTSPOT_Y_PICK,
                           UI_NAV_HOTSPOT_W, UI_NAV_HOTSPOT_H, LaserPage::Pick);
    shell->nav_hotspots[static_cast<int>(LaserPage::VoiceAi)] =
        create_nav_hotspot(shell, UI_NAV_HOTSPOT_X, UI_NAV_HOTSPOT_Y_VOICE,
                           UI_NAV_HOTSPOT_W, UI_NAV_HOTSPOT_H, LaserPage::VoiceAi);

    shell_raise_nav_layer(shell);
    shell_open_page(shell, LaserPage::Print, false);

    if (g_cnc_poll_timer == nullptr) {
        g_cnc_poll_timer = lv_timer_create(cnc_state_poll_cb, 250, shell);
    }
}
