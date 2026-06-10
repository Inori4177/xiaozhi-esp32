#include "page_voice_ai.h"

#include "../laser_ui_layout.h"
#include "../assets/ui_xiaozhi_walk_lr_frames.h"
#include "../assets/ui_xiaozhi_run_frames.h"
#include "../cnc/ui_cnc_print_service.h"

namespace {

enum class WalkDir {
    Right,
    Left,
};

lv_obj_t *g_anim_img = nullptr;
lv_timer_t *g_anim_timer = nullptr;
bool g_page_visible = false;
WalkDir g_dir = WalkDir::Right;
int g_pos_x = UI_XIAOZHI_WALK_MIN_X;
unsigned g_walk_frame_idx = 0;
unsigned g_run_frame_idx = 0;
ui_cnc_work_state_t g_last_cnc_state = UI_CNC_WORK_IDLE;

void apply_walk_sprite(WalkDir dir, unsigned frame)
{
    if (g_anim_img == nullptr || k_xiaozhi_walk_lr_frame_count == 0) {
        return;
    }
    if (frame >= k_xiaozhi_walk_lr_frame_count) {
        frame = 0;
    }

    const lv_image_dsc_t *src = (dir == WalkDir::Right) ? k_xiaozhi_walk_right_frames[frame]
                                                        : k_xiaozhi_walk_left_frames[frame];
    lv_obj_set_size(g_anim_img, UI_XIAOZHI_SPRITE_W, UI_XIAOZHI_SPRITE_H);
    lv_image_set_src(g_anim_img, src);
    lv_obj_set_pos(g_anim_img, g_pos_x, UI_XIAOZHI_WALK_Y);
}

void apply_run_sprite(unsigned frame)
{
    if (g_anim_img == nullptr || k_xiaozhi_run_frame_count == 0) {
        return;
    }
    if (frame >= k_xiaozhi_run_frame_count) {
        frame = 0;
    }

    lv_obj_set_size(g_anim_img, UI_XIAOZHI_RUN_SPRITE_W, UI_XIAOZHI_RUN_SPRITE_H);
    lv_image_set_src(g_anim_img, k_xiaozhi_run_frames[frame]);
    lv_obj_set_pos(g_anim_img, UI_XIAOZHI_RUN_CENTER_X, UI_XIAOZHI_RUN_CENTER_Y);
}

void reset_walk_state(void)
{
    g_dir = WalkDir::Right;
    g_pos_x = UI_XIAOZHI_WALK_MIN_X;
    g_walk_frame_idx = 0;
    apply_walk_sprite(WalkDir::Right, 0);
}

void show_idle_stopped_pose(void)
{
    g_dir = WalkDir::Right;
    g_pos_x = UI_XIAOZHI_WALK_MIN_X;
    g_walk_frame_idx = 0;
    apply_walk_sprite(WalkDir::Left, 0);
}

void on_cnc_state_changed(ui_cnc_work_state_t prev, ui_cnc_work_state_t next)
{
    if (next == UI_CNC_WORK_IDLE) {
        reset_walk_state();
        return;
    }
    if (prev == UI_CNC_WORK_IDLE && next == UI_CNC_WORK_RUNNING) {
        g_run_frame_idx = 0;
        apply_run_sprite(g_run_frame_idx);
    }
}

void tick_idle_walk(void)
{
    g_walk_frame_idx = (g_walk_frame_idx + 1) % k_xiaozhi_walk_lr_frame_count;

    if (g_dir == WalkDir::Right) {
        g_pos_x += UI_XIAOZHI_WALK_STEP_X;
        if (g_pos_x >= UI_XIAOZHI_WALK_MAX_X) {
            g_pos_x = UI_XIAOZHI_WALK_MAX_X;
            g_dir = WalkDir::Left;
            g_walk_frame_idx = 0;
        }
        apply_walk_sprite(WalkDir::Right, g_walk_frame_idx);
        return;
    }

    g_pos_x -= UI_XIAOZHI_WALK_STEP_X;
    if (g_pos_x <= UI_XIAOZHI_WALK_MIN_X) {
        g_pos_x = UI_XIAOZHI_WALK_MIN_X;
        g_dir = WalkDir::Right;
        g_walk_frame_idx = 0;
    }
    apply_walk_sprite(WalkDir::Left, g_walk_frame_idx);
}

void anim_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (!g_page_visible || g_anim_img == nullptr) {
        return;
    }

    ui_cnc_print_status_t st{};
    ui_cnc_print_service_get_status(&st);

    if (st.state != g_last_cnc_state) {
        on_cnc_state_changed(g_last_cnc_state, st.state);
        g_last_cnc_state = st.state;
    }

    switch (st.state) {
    case UI_CNC_WORK_IDLE:
        tick_idle_walk();
        break;
    case UI_CNC_WORK_RUNNING:
        g_run_frame_idx = (g_run_frame_idx + 1) % k_xiaozhi_run_frame_count;
        apply_run_sprite(g_run_frame_idx);
        break;
    case UI_CNC_WORK_PAUSED:
        apply_run_sprite(g_run_frame_idx);
        break;
    default:
        show_idle_stopped_pose();
        break;
    }
}

void sync_pose_for_current_state(void)
{
    ui_cnc_print_status_t st{};
    ui_cnc_print_service_get_status(&st);
    g_last_cnc_state = st.state;

    switch (st.state) {
    case UI_CNC_WORK_RUNNING:
    case UI_CNC_WORK_PAUSED:
        apply_run_sprite(g_run_frame_idx);
        break;
    case UI_CNC_WORK_IDLE:
        reset_walk_state();
        break;
    default:
        show_idle_stopped_pose();
        break;
    }
}

void anim_timer_start(void)
{
    if (g_anim_timer != nullptr) {
        lv_timer_resume(g_anim_timer);
        return;
    }
    g_anim_timer = lv_timer_create(anim_timer_cb, UI_XIAOZHI_WALK_FRAME_MS, nullptr);
}

void anim_timer_stop(void)
{
    if (g_anim_timer == nullptr) {
        return;
    }
    lv_timer_pause(g_anim_timer);
}

}  // namespace

lv_obj_t *page_voice_ai_create(lv_obj_t *parent)
{
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_set_size(page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(page, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(page, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(page, 0, LV_PART_MAIN);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);

    g_anim_img = lv_image_create(page);
    lv_obj_set_style_bg_opa(g_anim_img, LV_OPA_TRANSP, LV_PART_MAIN);
    g_run_frame_idx = 0;
    g_last_cnc_state = UI_CNC_WORK_IDLE;
    sync_pose_for_current_state();

    return page;
}

void page_voice_ai_on_show(void)
{
    g_page_visible = true;
    sync_pose_for_current_state();
    anim_timer_start();
}

void page_voice_ai_on_hide(void)
{
    g_page_visible = false;
    anim_timer_stop();
}
