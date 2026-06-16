#include "laser_ui_xiaozhi_presenter.h"

#include "laser_ui_layout.h"
#include "assets/laser_ui_images.h"
#include "assets/ui_xiaozhi_idle_blink_frames.h"
#include "assets/ui_xiaozhi_run_frames.h"
#include "assets/ui_xiaozhi_speak_frames.h"
#include "cnc/ui_cnc_print_service.h"
#include "../config.h"
#include "../peer_link/peer_voice_state.h"
#include "../peer_link/peer_voice_ui.h"

#include <cstring>

namespace {

constexpr uint32_t kPresenterTickMs = 100;
constexpr uint8_t kBlinkFrameHolds[] = {10, 1, 1, 1, 1, 1, 1, 14};
constexpr int kVoicePanelX = 12;
constexpr int kVoicePanelY = 16;
constexpr int kVoicePanelW = 318;
constexpr int kVoicePanelH = 236;
constexpr int kVoicePanelInset = 12;
constexpr int kVoiceSpriteRightMargin = 12;

lv_obj_t *g_root = nullptr;
lv_obj_t *g_pick_sprite = nullptr;
lv_obj_t *g_voice_page = nullptr;
lv_obj_t *g_voice_panel = nullptr;
lv_obj_t *g_voice_list = nullptr;
lv_obj_t *g_voice_sprite = nullptr;
lv_timer_t *g_timer = nullptr;
LaserPage g_current_page = LaserPage::Print;
unsigned g_blink_frame_idx = 0;
unsigned g_blink_hold = kBlinkFrameHolds[0];
unsigned g_run_frame_idx = 0;
unsigned g_speak_frame_idx = 0;
int g_last_chat_count = -1;

void hide_obj(lv_obj_t *obj)
{
    if (obj != nullptr) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

void show_obj(lv_obj_t *obj)
{
    if (obj != nullptr) {
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

void apply_pick_sprite(const lv_image_dsc_t *img, int w, int h)
{
    if (g_pick_sprite == nullptr || img == nullptr) {
        return;
    }
    show_obj(g_pick_sprite);
    lv_image_set_src(g_pick_sprite, img);
    lv_obj_set_size(g_pick_sprite, w, h);
    lv_obj_set_pos(g_pick_sprite, DISPLAY_WIDTH - w, DISPLAY_HEIGHT - h);
}

void apply_voice_sprite(const lv_image_dsc_t *img)
{
    if (g_voice_sprite == nullptr || img == nullptr) {
        return;
    }
    show_obj(g_voice_sprite);
    lv_image_set_src(g_voice_sprite, img);
    lv_obj_set_size(g_voice_sprite, UI_XIAOZHI_SPEAK_W, UI_XIAOZHI_SPEAK_H);
    lv_obj_set_pos(g_voice_sprite, UI_CONTENT_W - UI_XIAOZHI_SPEAK_W - kVoiceSpriteRightMargin,
                   (UI_MAIN_H - UI_XIAOZHI_SPEAK_H) / 2);
}

void update_pick_sprite(ui_cnc_work_state_t cnc_state)
{
    if (g_current_page != LaserPage::Pick) {
        hide_obj(g_pick_sprite);
        return;
    }

    if (cnc_state == UI_CNC_WORK_RUNNING) {
        g_run_frame_idx = (g_run_frame_idx + 1) % k_xiaozhi_run_frame_count;
        apply_pick_sprite(k_xiaozhi_run_frames[g_run_frame_idx], UI_XIAOZHI_RUN_SPRITE_W,
                          UI_XIAOZHI_RUN_SPRITE_H);
        return;
    }

    if (cnc_state == UI_CNC_WORK_PAUSED) {
        apply_pick_sprite(k_xiaozhi_run_frames[g_run_frame_idx], UI_XIAOZHI_RUN_SPRITE_W,
                          UI_XIAOZHI_RUN_SPRITE_H);
        return;
    }

    if (g_blink_hold > 0) {
        --g_blink_hold;
    }
    if (g_blink_hold == 0) {
        g_blink_frame_idx = (g_blink_frame_idx + 1) % k_xiaozhi_idle_blink_frame_count;
        g_blink_hold = kBlinkFrameHolds[g_blink_frame_idx];
    }
    apply_pick_sprite(k_xiaozhi_idle_blink_frames[g_blink_frame_idx], UI_XIAOZHI_IDLE_BLINK_W,
                      UI_XIAOZHI_IDLE_BLINK_H);
}

void clear_children(lv_obj_t *obj)
{
    if (obj == nullptr) {
        return;
    }
    while (lv_obj_get_child_count(obj) > 0) {
        lv_obj_t *child = lv_obj_get_child(obj, 0);
        if (child == nullptr) {
            break;
        }
        lv_obj_del(child);
    }
}

void rebuild_voice_list(void)
{
    if (g_voice_list == nullptr) {
        return;
    }

    const int count = peer_voice_ui_get_chat_count();
    if (count == g_last_chat_count) {
        return;
    }
    g_last_chat_count = count;
    clear_children(g_voice_list);

    for (int i = 0; i < count; ++i) {
        const char *role = peer_voice_ui_get_chat_role(i);
        const char *text = peer_voice_ui_get_chat_text(i);
        const bool is_user = role != nullptr && strcmp(role, "user") == 0;

        lv_obj_t *row = lv_obj_create(g_voice_list);
        lv_obj_set_width(row, LV_PCT(100));
        lv_obj_set_height(row, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_bottom(row, 6, LV_PART_MAIN);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *bubble = lv_obj_create(row);
        lv_obj_set_width(bubble, LV_PCT(82));
        lv_obj_set_height(bubble, LV_SIZE_CONTENT);
        lv_obj_set_style_radius(bubble, 8, LV_PART_MAIN);
        lv_obj_set_style_bg_color(bubble, is_user ? UI_COLOR_SOFT_MINT : UI_COLOR_CARD, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(bubble, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(bubble, is_user ? UI_COLOR_MINT : UI_COLOR_SOFT_MINT, LV_PART_MAIN);
        lv_obj_set_style_border_width(bubble, 1, LV_PART_MAIN);
        lv_obj_set_style_pad_left(bubble, 10, LV_PART_MAIN);
        lv_obj_set_style_pad_right(bubble, 10, LV_PART_MAIN);
        lv_obj_set_style_pad_top(bubble, 8, LV_PART_MAIN);
        lv_obj_set_style_pad_bottom(bubble, 8, LV_PART_MAIN);
        lv_obj_clear_flag(bubble, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_align(bubble, is_user ? LV_ALIGN_TOP_LEFT : LV_ALIGN_TOP_RIGHT, 0, 0);

        lv_obj_t *label = lv_label_create(bubble);
        lv_obj_set_width(label, LV_PCT(100));
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_color(label, UI_COLOR_TEXT, LV_PART_MAIN);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_label_set_text(label, text != nullptr ? text : "");

    }

    lv_obj_scroll_to_y(g_voice_list, LV_COORD_MAX, LV_ANIM_OFF);
}

void update_voice_page(void)
{
    if (g_current_page != LaserPage::VoiceAi || g_voice_page == nullptr) {
        hide_obj(g_voice_panel);
        hide_obj(g_voice_sprite);
        return;
    }

    show_obj(g_voice_panel);
    rebuild_voice_list();

    const char *voice_state = peer_voice_state_get();
    const bool speaking = voice_state != nullptr && strcmp(voice_state, "speaking") == 0;
    if (!speaking) {
        g_speak_frame_idx = 0;
        apply_voice_sprite(&speak_5close);
        return;
    }

    g_speak_frame_idx = (g_speak_frame_idx + 1) % k_xiaozhi_speak_loop_frame_count;
    apply_voice_sprite(k_xiaozhi_speak_loop_frames[g_speak_frame_idx]);
}

void timer_cb(lv_timer_t *timer)
{
    (void)timer;
    ui_cnc_print_status_t st{};
    ui_cnc_print_service_get_status(&st);
    update_pick_sprite(st.state);
    update_voice_page();
}

}  // namespace

void laser_ui_xiaozhi_presenter_init(lv_obj_t *screen)
{
    if (screen == nullptr || g_root != nullptr) {
        return;
    }

    g_root = screen;
    g_pick_sprite = lv_image_create(screen);
    lv_obj_set_style_bg_opa(g_pick_sprite, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_clear_flag(g_pick_sprite, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(g_pick_sprite, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(g_pick_sprite);

    if (g_timer == nullptr) {
        g_timer = lv_timer_create(timer_cb, kPresenterTickMs, nullptr);
    }
}

void laser_ui_xiaozhi_presenter_bind_voice_page(lv_obj_t *page)
{
    if (page == nullptr || g_voice_page != nullptr) {
        return;
    }

    g_voice_page = page;
    g_voice_panel = lv_obj_create(page);
    lv_obj_set_pos(g_voice_panel, kVoicePanelX, kVoicePanelY);
    lv_obj_set_size(g_voice_panel, kVoicePanelW, kVoicePanelH);
    lv_obj_set_style_radius(g_voice_panel, 8, LV_PART_MAIN);
    lv_obj_set_style_bg_color(g_voice_panel, UI_COLOR_CARD, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(g_voice_panel, LV_OPA_90, LV_PART_MAIN);
    lv_obj_set_style_border_color(g_voice_panel, UI_COLOR_SOFT_MINT, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_voice_panel, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_voice_panel, kVoicePanelInset, LV_PART_MAIN);
    lv_obj_clear_flag(g_voice_panel, LV_OBJ_FLAG_SCROLLABLE);

    g_voice_list = lv_obj_create(g_voice_panel);
    lv_obj_set_size(g_voice_list, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(g_voice_list, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_voice_list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_voice_list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(g_voice_list, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(g_voice_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g_voice_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scrollbar_mode(g_voice_list, LV_SCROLLBAR_MODE_OFF);

    g_voice_sprite = lv_image_create(page);
    lv_obj_set_style_bg_opa(g_voice_sprite, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_clear_flag(g_voice_sprite, LV_OBJ_FLAG_CLICKABLE);
    hide_obj(g_voice_sprite);
    hide_obj(g_voice_panel);
}

void laser_ui_xiaozhi_presenter_set_current_page(LaserPage page)
{
    g_current_page = page;
    if (page == LaserPage::VoiceAi) {
        g_last_chat_count = -1;
    }
    timer_cb(nullptr);
}

void laser_ui_xiaozhi_presenter_raise_overlay(void)
{
    if (g_pick_sprite != nullptr) {
        lv_obj_move_foreground(g_pick_sprite);
    }
}
