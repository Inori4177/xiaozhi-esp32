#include "ui_cnc_print_service.h"

#include "../../laser_ui_state.h"
#include "ui_cnc_config.h"
#include "ui_cnc_coord_map.h"
#include "ui_cnc_motion_facade.h"

#include "gcode_parser.h"
#include "planner.h"
#include "stepper.h"
#include "stepping_engine.h"

#include <cmath>
#include <cstring>
#include <string>

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

static const char *TAG = "ui_cnc_print";

static float s_feed_mm_min = static_cast<float>(UI_CNC_FEED_BASE_MM_MIN);
static volatile bool s_busy = false;
static QueueHandle_t s_work_queue = nullptr;
static TaskHandle_t s_worker_task = nullptr;

typedef enum {
    CNC_WORK_GCODE = 0,
    CNC_WORK_JOG,
    CNC_WORK_HOME,
    CNC_WORK_LASER_OFF,
} cnc_work_id_t;

typedef struct {
    cnc_work_id_t id;
    float x;
    float y;
    char gcode[UI_CNC_WORK_GCODE_MAX];
} cnc_work_msg_t;

static void cnc_worker_task(void *arg);

static int power_pct_to_spindle(int pct)
{
    int s = pct * 10;
    if (s < 0) {
        s = 0;
    }
    if (s > 1000) {
        s = 1000;
    }
    return s;
}

static int speed_pct_to_feed_mm_min(int speed_pct)
{
    int feed = UI_CNC_FEED_BASE_MM_MIN * speed_pct / 100;
    if (feed < 1) {
        feed = 1;
    }
    return feed;
}

static void clamp_target_mm(float *x_mm, float *y_mm)
{
    ui_cnc_clamp_mm(x_mm, y_mm);
}

static bool move_linear_mm(float target_x, float target_y, float feed_mm_min, bool rapid)
{
    clamp_target_mm(&target_x, &target_y);

    float cur_x = 0.0f;
    float cur_y = 0.0f;
    ui_cnc_motion_facade_get_position_mm(&cur_x, &cur_y);

    const float dx = target_x - cur_x;
    const float dy = target_y - cur_y;
    const float mm = sqrtf(dx * dx + dy * dy);
    const float rate = rapid ? UI_CNC_RAPID_FEED_MM_MIN : feed_mm_min;

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
        block.programmed_rate = rate;

        Stepper::SubmitBlock(&block);
        Stepper::WaitForIdle();
    }

    ui_cnc_motion_facade_set_position_mm(target_x, target_y);
    Stepper::GoIdle();
    return true;
}

static void set_laser_spindle(int spindle)
{
    ui_cnc_motion_facade_init();
    if (spindle > 0) {
        SteppingEngine::SetLaser(static_cast<uint32_t>(spindle));
        ESP_LOGI(TAG, "Laser PWM S%d (%.1f%%)", spindle, spindle / 10.0f);
    } else {
        SteppingEngine::LaserOff();
        ESP_LOGI(TAG, "Laser OFF");
    }
}

static void exec_parsed_commands(const std::vector<GCodeCommand> &commands)
{
    ui_cnc_motion_facade_init();

    for (const auto &cmd : commands) {
        switch (cmd.type) {
        case GCodeCommand::G0: {
            float tx = cmd.x;
            float ty = cmd.y;
            move_linear_mm(tx, ty, UI_CNC_RAPID_FEED_MM_MIN, true);
            break;
        }
        case GCodeCommand::G1: {
            if (cmd.feed_rate > 0.0f) {
                s_feed_mm_min = cmd.feed_rate;
                ESP_LOGI(TAG, "Feed F%.0f mm/min", static_cast<double>(s_feed_mm_min));
            }
            float cur_x = 0.0f;
            float cur_y = 0.0f;
            ui_cnc_motion_facade_get_position_mm(&cur_x, &cur_y);
            if (fabsf(cmd.x - cur_x) > 0.001f || fabsf(cmd.y - cur_y) > 0.001f) {
                move_linear_mm(cmd.x, cmd.y, s_feed_mm_min, false);
            }
            break;
        }
        case GCodeCommand::M3:
            set_laser_spindle(cmd.spindle);
            break;
        case GCodeCommand::M5:
            set_laser_spindle(0);
            break;
        case GCodeCommand::G21:
        case GCodeCommand::G90:
            break;
        default:
            break;
        }
    }
}

static void execute_gcode_blocking(const char *gcode_text)
{
    if (gcode_text == nullptr || gcode_text[0] == '\0') {
        return;
    }

    const auto commands = GCodeParser::Parse(std::string(gcode_text));
    ESP_LOGI(TAG, "Execute %d parsed commands", static_cast<int>(commands.size()));
    exec_parsed_commands(commands);
}

static bool ensure_worker_started(void)
{
    if (s_worker_task != nullptr) {
        return true;
    }
    if (s_work_queue == nullptr) {
        return false;
    }
    if (xTaskCreatePinnedToCore(cnc_worker_task, "ui_cnc_worker", UI_CNC_WORK_TASK_STACK, nullptr,
                                UI_CNC_MOVE_TASK_PRIO, &s_worker_task, UI_CNC_TASK_CORE) != pdPASS) {
        ESP_LOGE(TAG, "failed to create worker task");
        return false;
    }
    ESP_LOGI(TAG, "CNC worker started on core %d", UI_CNC_TASK_CORE);
    return true;
}

static bool post_work(const cnc_work_msg_t *msg)
{
    if (s_work_queue == nullptr || msg == nullptr) {
        return false;
    }
    if (!ensure_worker_started()) {
        ESP_LOGW(TAG, "CNC worker not ready, drop id=%d", static_cast<int>(msg->id));
        return false;
    }
    if (xQueueSend(s_work_queue, msg, 0) != pdTRUE) {
        ESP_LOGW(TAG, "CNC work queue full, drop id=%d", static_cast<int>(msg->id));
        return false;
    }
    return true;
}

static bool post_gcode_async(const char *gcode_text)
{
    if (gcode_text == nullptr || gcode_text[0] == '\0') {
        return false;
    }
    cnc_work_msg_t msg = {};
    msg.id = CNC_WORK_GCODE;
    strncpy(msg.gcode, gcode_text, sizeof(msg.gcode) - 1);
    msg.gcode[sizeof(msg.gcode) - 1] = '\0';
    return post_work(&msg);
}

static void cnc_worker_task(void *arg)
{
    (void)arg;
    cnc_work_msg_t msg;

    ui_cnc_motion_facade_init();

    for (;;) {
        if (xQueueReceive(s_work_queue, &msg, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        s_busy = true;
        switch (msg.id) {
        case CNC_WORK_GCODE:
            execute_gcode_blocking(msg.gcode);
            break;
        case CNC_WORK_JOG:
            ui_cnc_motion_facade_init();
            move_linear_mm(msg.x, msg.y, UI_CNC_RAPID_FEED_MM_MIN, true);
            break;
        case CNC_WORK_HOME:
            ui_cnc_motion_facade_init();
            move_linear_mm(0.0f, 0.0f, UI_CNC_RAPID_FEED_MM_MIN, true);
            break;
        case CNC_WORK_LASER_OFF:
            execute_gcode_blocking("M5");
            Stepper::GoIdle();
            ESP_LOGI(TAG, "pause: laser off");
            break;
        default:
            break;
        }
        s_busy = false;
    }
}

void ui_cnc_print_service_execute_gcode(const char *gcode_text)
{
    (void)post_gcode_async(gcode_text);
}

bool ui_cnc_print_service_is_busy(void)
{
    return s_busy;
}

void ui_cnc_print_service_move_to_mm_async(float x_mm, float y_mm)
{
    cnc_work_msg_t msg = {};
    msg.id = CNC_WORK_JOG;
    msg.x = x_mm;
    msg.y = y_mm;
    clamp_target_mm(&msg.x, &msg.y);
    post_work(&msg);
}

void ui_cnc_print_service_home_async(void)
{
    cnc_work_msg_t msg = {};
    msg.id = CNC_WORK_HOME;
    post_work(&msg);
}

static void jog_axis_async(char axis, bool positive)
{
    float x = 0.0f;
    float y = 0.0f;
    ui_cnc_motion_facade_get_position_mm(&x, &y);

    const float step = laser_ui_state_get_jog_step_mm();
    if (axis == 'X' || axis == 'x') {
        x += positive ? step : -step;
    } else {
        y += positive ? step : -step;
    }
    clamp_target_mm(&x, &y);

    cnc_work_msg_t msg = {};
    msg.id = CNC_WORK_JOG;
    msg.x = x;
    msg.y = y;
    post_work(&msg);
}

static void apply_settings_from_ui(void)
{
    const laser_ui_settings_t s = laser_ui_state_get_settings();
    const int spindle = power_pct_to_spindle(s.laser_power_pct);
    s_feed_mm_min = static_cast<float>(speed_pct_to_feed_mm_min(s.speed_pct));

    char gcode[96];
    snprintf(gcode, sizeof(gcode), "G21\nG90\nM3 S%d\nG1 F%d", spindle,
             static_cast<int>(s_feed_mm_min));
    post_gcode_async(gcode);
}

void ui_cnc_print_service_on_event(laser_ui_event_id_t id)
{
    switch (id) {
    case LASER_EVT_JOG_X_PLUS:
        jog_axis_async('X', true);
        break;
    case LASER_EVT_JOG_X_MINUS:
        jog_axis_async('X', false);
        break;
    case LASER_EVT_JOG_Y_PLUS:
        jog_axis_async('Y', true);
        break;
    case LASER_EVT_JOG_Y_MINUS:
        jog_axis_async('Y', false);
        break;
    case LASER_EVT_JOG_HOME: {
        cnc_work_msg_t msg = {};
        msg.id = CNC_WORK_HOME;
        post_work(&msg);
        break;
    }
    case LASER_EVT_RUN: {
        const laser_ui_settings_t s = laser_ui_state_get_settings();
        const int spindle = power_pct_to_spindle(s.laser_power_pct);
        s_feed_mm_min = static_cast<float>(speed_pct_to_feed_mm_min(s.speed_pct));
        char gcode[64];
        snprintf(gcode, sizeof(gcode), "M3 S%d\nG1 F%d", spindle, static_cast<int>(s_feed_mm_min));
        post_gcode_async(gcode);
        ESP_LOGI(TAG, "run queued: laser on, feed %.0f mm/min", static_cast<double>(s_feed_mm_min));
        break;
    }
    case LASER_EVT_PAUSE: {
        cnc_work_msg_t msg = {};
        msg.id = CNC_WORK_LASER_OFF;
        post_work(&msg);
        break;
    }
    case LASER_EVT_SETTINGS_APPLY:
        ESP_LOGI(TAG, "settings apply queued → local CNC");
        apply_settings_from_ui();
        break;
    case LASER_EVT_STEP_CHANGED:
        ESP_LOGD(TAG, "jog step %.1f mm", static_cast<double>(laser_ui_state_get_jog_step_mm()));
        break;
    case LASER_EVT_POWER_CHANGED:
    case LASER_EVT_SPEED_CHANGED:
        ESP_LOGD(TAG, "settings preview (apply to send to CNC)");
        break;
    default:
        break;
    }
}

void ui_cnc_print_service_init(void)
{
    if (s_work_queue != nullptr) {
        return;
    }

    s_work_queue = xQueueCreate(UI_CNC_WORK_QUEUE_LEN, sizeof(cnc_work_msg_t));
    if (s_work_queue == nullptr) {
        ESP_LOGE(TAG, "failed to create work queue");
        return;
    }

    ESP_LOGI(TAG, "CNC queue ready (worker starts on first move)");
}
