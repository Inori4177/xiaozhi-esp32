#include "ui_cnc_motion_facade.h"

#include "ui_cnc_config.h"

#include "motion_controller.h"
#include "planner.h"
#include "stepper.h"
#include "stepping_engine.h"

#include <cmath>
#include <cstring>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "ui_cnc_motion";

static float s_logical_x = 0.0f;
static float s_logical_y = 0.0f;
static bool s_facade_ready = false;
static volatile bool s_moving = false;

static bool s_segment_active = false;
static float s_seg_from_x = 0.0f;
static float s_seg_from_y = 0.0f;
static float s_seg_to_x = 0.0f;
static float s_seg_to_y = 0.0f;

static void ensure_motion_init(void)
{
    if (s_facade_ready) {
        return;
    }
    MotionController::GlobalInit(UI_CNC_STEPS_PER_MM_X,
                                 UI_CNC_STEPS_PER_MM_Y,
                                 UI_CNC_MAX_RATE_MM_MIN,
                                 UI_CNC_ACCELERATION,
                                 UI_CNC_JUNCTION_DEV,
                                 UI_CNC_MAX_TRAVEL_MM);
    s_logical_x = 0.0f;
    s_logical_y = 0.0f;
    s_facade_ready = true;
    ESP_LOGI(TAG, "Motion facade ready (logical 0,0)");
}

static bool move_linear_mm(float target_x, float target_y, float feed_mm_min)
{
    (void)feed_mm_min;
    if (target_x < 0.0f || target_y < 0.0f ||
        target_x > UI_CNC_WORK_SIZE_MM || target_y > UI_CNC_WORK_SIZE_MM) {
        ESP_LOGW(TAG, "Target (%.2f, %.2f) out of work area", target_x, target_y);
        return false;
    }

    const float dx = target_x - s_logical_x;
    const float dy = target_y - s_logical_y;
    const float mm = sqrtf(dx * dx + dy * dy);

    if (mm > 0.001f) {
        PlanBlock block;
        memset(&block, 0, sizeof(block));
        block.millimeters = mm;
        const int32_t sx = static_cast<int32_t>(lroundf(dx * Planner::steps_per_mm_x));
        const int32_t sy = static_cast<int32_t>(lroundf(dy * Planner::steps_per_mm_y));
        block.steps[0] = static_cast<uint32_t>(abs(sx));
        block.steps[1] = static_cast<uint32_t>(abs(sy));
        block.step_event_count = std::max(block.steps[0], block.steps[1]);
        block.direction_bits = 0;
        if (sx < 0) {
            block.direction_bits |= 0x01;
        }
        if (sy < 0) {
            block.direction_bits |= 0x02;
        }
        block.programmed_rate = feed_mm_min;

        ui_cnc_motion_facade_begin_segment_mm(s_logical_x, s_logical_y, target_x, target_y, feed_mm_min);
        Stepper::SubmitBlock(&block);
        Stepper::WaitForIdle();
    }

    ui_cnc_motion_facade_end_segment_mm(target_x, target_y);
    Stepper::GoIdle();
    return true;
}

void ui_cnc_motion_facade_init(void)
{
    ensure_motion_init();
}

bool ui_cnc_motion_facade_is_ready(void)
{
    return s_facade_ready;
}

void ui_cnc_motion_facade_get_position_mm(float *x_mm, float *y_mm)
{
    if (x_mm != nullptr) {
        *x_mm = s_logical_x;
    }
    if (y_mm != nullptr) {
        *y_mm = s_logical_y;
    }
}

void ui_cnc_motion_facade_get_display_position_mm(float *x_mm, float *y_mm)
{
    if (x_mm == nullptr && y_mm == nullptr) {
        return;
    }

    float dx = s_logical_x;
    float dy = s_logical_y;
    if (Stepper::IsBusy()) {
        if (s_segment_active) {
            const float prog = Stepper::GetBlockProgress();
            dx = s_seg_from_x + (s_seg_to_x - s_seg_from_x) * prog;
            dy = s_seg_from_y + (s_seg_to_y - s_seg_from_y) * prog;
        }
    } else if (s_segment_active) {
        /* 运动已停但段标记未清（暂停/Abort 后）— 用逻辑坐标并清除 stale 段。 */
        s_segment_active = false;
    }

    if (x_mm != nullptr) {
        *x_mm = dx;
    }
    if (y_mm != nullptr) {
        *y_mm = dy;
    }
}

void ui_cnc_motion_facade_set_position_mm(float x_mm, float y_mm)
{
    s_logical_x = x_mm;
    s_logical_y = y_mm;
    s_segment_active = false;
}

void ui_cnc_motion_facade_begin_segment_mm(float from_x, float from_y, float to_x, float to_y,
                                           float feed_mm_min)
{
    (void)feed_mm_min;
    s_seg_from_x = from_x;
    s_seg_from_y = from_y;
    s_seg_to_x = to_x;
    s_seg_to_y = to_y;
    s_segment_active = true;
}

void ui_cnc_motion_facade_end_segment_mm(float x_mm, float y_mm)
{
    s_logical_x = x_mm;
    s_logical_y = y_mm;
    s_segment_active = false;
}

bool ui_cnc_motion_facade_rapid_to_mm(float x_mm, float y_mm)
{
    ensure_motion_init();
    return move_linear_mm(x_mm, y_mm, UI_CNC_RAPID_FEED_MM_MIN);
}

struct MoveTaskCtx {
    float x;
    float y;
};

static void move_task(void *arg)
{
    auto *ctx = static_cast<MoveTaskCtx *>(arg);
    s_moving = true;
    ensure_motion_init();
    move_linear_mm(ctx->x, ctx->y, UI_CNC_RAPID_FEED_MM_MIN);
    s_moving = false;
    delete ctx;
    vTaskDelete(nullptr);
}

void ui_cnc_motion_facade_rapid_to_mm_async(float x_mm, float y_mm)
{
    auto *ctx = new MoveTaskCtx{x_mm, y_mm};
    if (xTaskCreatePinnedToCore(move_task, "ui_pick_move", UI_CNC_MOVE_TASK_STACK, ctx,
                                UI_CNC_MOVE_TASK_PRIO, nullptr, UI_CNC_TASK_CORE) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create move task");
        delete ctx;
    }
}

bool ui_cnc_motion_facade_is_moving(void)
{
    return s_moving;
}
