#include "ui_cnc_print_service.h"

#include "../../laser_ui_state.h"
#include "ui_cnc_config.h"
#include "ui_cnc_coord_map.h"
#include "ui_cnc_motion_facade.h"

#include "gcode_parser.h"
#include "planner.h"
#include "stepper.h"
#include "stepping_engine.h"
#include "motion_controller.h"

#include <cstdio>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

static const char *TAG = "ui_cnc_print";

static float s_feed_mm_min = static_cast<float>(UI_CNC_FEED_BASE_MM_MIN);
static volatile bool s_busy = false;
static QueueHandle_t s_work_queue = nullptr;
static TaskHandle_t s_worker_task = nullptr;
static char s_job_name[64] = {};

static volatile ui_cnc_work_state_t s_work_state = UI_CNC_WORK_IDLE;
static volatile uint8_t s_progress_pct = 0;
static volatile uint32_t s_elapsed_sec = 0;
static volatile uint32_t s_eta_sec = 0;
static volatile bool s_has_eta = false;
static volatile bool s_is_print_job = false;
static volatile bool s_paused = false;

static int64_t s_job_start_us = 0;
static int s_job_total_cmds = 0;
static int s_job_done_cmds = 0;
static float s_job_total_est_sec = 0.0f;

typedef enum {
    CNC_WORK_GCODE = 0,
    CNC_WORK_JOG,
    CNC_WORK_HOME,
    CNC_WORK_LASER_OFF,
    CNC_WORK_FILE,
} cnc_work_id_t;

typedef struct {
    cnc_work_id_t id;
    float x;
    float y;
    char gcode[UI_CNC_WORK_GCODE_MAX];
    char file_path[128];
} cnc_work_msg_t;

static void cnc_worker_task(void *arg);

static void set_job_name_from_path(const char *path)
{
    if (path == nullptr || path[0] == '\0') {
        snprintf(s_job_name, sizeof(s_job_name), "G-code");
        return;
    }
    const char *base = strrchr(path, '/');
    base = (base != nullptr) ? base + 1 : path;
    strncpy(s_job_name, base, sizeof(s_job_name) - 1);
    s_job_name[sizeof(s_job_name) - 1] = '\0';
}

static void reset_job_status(void)
{
    s_work_state = UI_CNC_WORK_IDLE;
    s_progress_pct = 0;
    s_elapsed_sec = 0;
    s_eta_sec = 0;
    s_has_eta = false;
    s_is_print_job = false;
    s_paused = false;
    s_job_start_us = 0;
    s_job_total_cmds = 0;
    s_job_done_cmds = 0;
    s_job_total_est_sec = 0.0f;
}

static void refresh_elapsed_locked(void)
{
    if (s_job_start_us <= 0) {
        s_elapsed_sec = 0;
        return;
    }
    const int64_t now = esp_timer_get_time();
    const int64_t elapsed_us = now - s_job_start_us;
    s_elapsed_sec = static_cast<uint32_t>(elapsed_us / 1000000LL);
}

static void refresh_progress_locked(void)
{
    refresh_elapsed_locked();

    if (s_job_total_cmds <= 0) {
        s_progress_pct = 0;
        s_has_eta = false;
        return;
    }

    if (s_job_done_cmds >= s_job_total_cmds) {
        s_progress_pct = 100;
        s_eta_sec = 0;
        s_has_eta = true;
        return;
    }

    s_progress_pct = static_cast<uint8_t>((s_job_done_cmds * 100) / s_job_total_cmds);
    if (s_job_done_cmds > 0 && s_elapsed_sec > 0) {
        const uint32_t total_est = static_cast<uint32_t>(
            (static_cast<float>(s_elapsed_sec) * static_cast<float>(s_job_total_cmds)) /
            static_cast<float>(s_job_done_cmds));
        s_eta_sec = (total_est > s_elapsed_sec) ? (total_est - s_elapsed_sec) : 0;
        s_has_eta = true;
    } else if (s_job_total_est_sec > 0.0f) {
        const float ratio = static_cast<float>(s_job_done_cmds) / static_cast<float>(s_job_total_cmds);
        const uint32_t total_est = static_cast<uint32_t>(s_job_total_est_sec);
        const uint32_t elapsed = static_cast<uint32_t>(s_job_total_est_sec * ratio);
        s_eta_sec = (total_est > elapsed) ? (total_est - elapsed) : 0;
        s_has_eta = true;
    } else {
        s_has_eta = false;
    }
}

static void estimate_commands(const std::vector<GCodeCommand> &commands, int *out_total_cmds,
                              float *out_total_sec)
{
    if (out_total_cmds != nullptr) {
        *out_total_cmds = static_cast<int>(commands.size());
    }
    if (out_total_sec == nullptr) {
        return;
    }

    float pos_x = 0.0f;
    float pos_y = 0.0f;
    float feed = s_feed_mm_min;
    float total_sec = 0.0f;

    for (const auto &cmd : commands) {
        switch (cmd.type) {
        case GCodeCommand::G1:
            if (cmd.feed_rate > 0.0f) {
                feed = cmd.feed_rate;
            }
            /* fallthrough */
        case GCodeCommand::G0: {
            const float tx = std::isnan(cmd.x) ? pos_x : cmd.x;
            const float ty = std::isnan(cmd.y) ? pos_y : cmd.y;
            const float dx = tx - pos_x;
            const float dy = ty - pos_y;
            const float mm = sqrtf(dx * dx + dy * dy);
            if (mm > 0.001f) {
                const float rate =
                    (cmd.type == GCodeCommand::G0) ? UI_CNC_RAPID_FEED_MM_MIN : feed;
                const float safe_rate = (rate < 1.0f) ? 1.0f : rate;
                total_sec += (mm / safe_rate) * 60.0f;
            }
            pos_x = tx;
            pos_y = ty;
            break;
        }
        default:
            break;
        }
    }

    *out_total_sec = total_sec;
}

static void begin_tracked_job(const std::vector<GCodeCommand> &commands, bool print_job)
{
    s_job_start_us = esp_timer_get_time();
    s_job_done_cmds = 0;
    estimate_commands(commands, &s_job_total_cmds, &s_job_total_est_sec);
    s_is_print_job = print_job;
    s_paused = false;
    s_work_state = print_job ? UI_CNC_WORK_RUNNING : UI_CNC_WORK_JOGGING;
    refresh_progress_locked();
}

static void motion_progress_cb(size_t done, size_t total, void *user_data)
{
    (void)user_data;
    s_job_total_cmds = static_cast<int>(total);
    s_job_done_cmds = static_cast<int>(done);
    refresh_progress_locked();
}

static void bind_motion_progress_callback(void)
{
    static bool bound = false;
    if (bound) {
        return;
    }
    MotionController::SetCommandProgressCallback(motion_progress_cb, nullptr);
    bound = true;
}

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

    const size_t total = commands.size();
    for (size_t i = 0; i < total; ++i) {
        const auto &cmd = commands[i];
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
        ui_cnc_print_service_notify_job_progress(i + 1, total);
    }
}

static void execute_gcode_blocking(const char *gcode_text)
{
    if (gcode_text == nullptr || gcode_text[0] == '\0') {
        return;
    }
    set_job_name_from_path("G-code");

    const auto commands = GCodeParser::Parse(std::string(gcode_text));
    ESP_LOGI(TAG, "Execute %d parsed commands", static_cast<int>(commands.size()));
    begin_tracked_job(commands, true);
    exec_parsed_commands(commands);
}

static void execute_gcode_file_blocking(const char *path)
{
    if (path == nullptr || path[0] == '\0') {
        return;
    }
    set_job_name_from_path(path);
    FILE *f = fopen(path, "rb");
    if (f == nullptr) {
        ESP_LOGE(TAG, "open failed: %s", path);
        return;
    }
    std::string chunk;
    chunk.reserve(4096);
    char line[256];
    std::vector<GCodeCommand> all_commands;
    while (fgets(line, sizeof(line), f) != nullptr) {
        chunk.append(line);
        if (chunk.size() >= 3500) {
            const auto part = GCodeParser::Parse(chunk);
            all_commands.insert(all_commands.end(), part.begin(), part.end());
            chunk.clear();
        }
    }
    fclose(f);
    if (!chunk.empty()) {
        const auto part = GCodeParser::Parse(chunk);
        all_commands.insert(all_commands.end(), part.begin(), part.end());
    }
    ESP_LOGI(TAG, "File %s -> %d commands", path, static_cast<int>(all_commands.size()));
    begin_tracked_job(all_commands, true);
    exec_parsed_commands(all_commands);
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
            s_work_state = UI_CNC_WORK_JOGGING;
            s_is_print_job = false;
            s_job_start_us = esp_timer_get_time();
            s_job_total_cmds = 1;
            s_job_done_cmds = 0;
            s_progress_pct = 0;
            s_has_eta = false;
            move_linear_mm(msg.x, msg.y, UI_CNC_RAPID_FEED_MM_MIN, true);
            s_job_done_cmds = 1;
            s_progress_pct = 100;
            refresh_elapsed_locked();
            break;
        case CNC_WORK_HOME:
            ui_cnc_motion_facade_init();
            s_work_state = UI_CNC_WORK_JOGGING;
            s_is_print_job = false;
            s_job_start_us = esp_timer_get_time();
            s_job_total_cmds = 1;
            s_job_done_cmds = 0;
            s_progress_pct = 0;
            s_has_eta = false;
            move_linear_mm(0.0f, 0.0f, UI_CNC_RAPID_FEED_MM_MIN, true);
            s_job_done_cmds = 1;
            s_progress_pct = 100;
            refresh_elapsed_locked();
            break;
        case CNC_WORK_LASER_OFF:
            ui_cnc_motion_facade_init();
            s_paused = true;
            s_work_state = UI_CNC_WORK_PAUSED;
            set_laser_spindle(0);
            Stepper::GoIdle();
            refresh_elapsed_locked();
            ESP_LOGI(TAG, "pause: laser off");
            break;
        case CNC_WORK_FILE:
            execute_gcode_file_blocking(msg.file_path);
            break;
        default:
            break;
        }
        s_busy = false;
        if (msg.id == CNC_WORK_GCODE || msg.id == CNC_WORK_FILE) {
            s_work_state = UI_CNC_WORK_IDLE;
            s_progress_pct = 100;
            refresh_elapsed_locked();
            s_has_eta = true;
            s_eta_sec = 0;
        } else if (msg.id == CNC_WORK_JOG || msg.id == CNC_WORK_HOME) {
            reset_job_status();
        }
    }
}

bool ui_cnc_print_service_worker_ready(void)
{
    return s_worker_task != nullptr;
}

bool ui_cnc_print_service_execute_gcode(const char *gcode_text)
{
    return post_gcode_async(gcode_text);
}

bool ui_cnc_print_service_is_busy(void)
{
    return s_busy;
}

const char *ui_cnc_print_service_get_job_name(void)
{
    return s_job_name[0] != '\0' ? s_job_name : "-";
}

void ui_cnc_print_service_get_status(ui_cnc_print_status_t *out)
{
    if (out == nullptr) {
        return;
    }

    if (s_busy || s_work_state != UI_CNC_WORK_IDLE) {
        refresh_progress_locked();
    }

    out->state = s_work_state;
    out->progress_pct = s_progress_pct;
    out->elapsed_sec = s_elapsed_sec;
    out->eta_sec = s_eta_sec;
    out->has_eta = s_has_eta;
    out->is_print_job = s_is_print_job;
}

void ui_cnc_print_service_notify_job_begin(const char *gcode_text)
{
    if (gcode_text == nullptr || gcode_text[0] == '\0') {
        return;
    }

    s_busy = true;
    const auto commands = GCodeParser::Parse(std::string(gcode_text));
    begin_tracked_job(commands, true);
}

void ui_cnc_print_service_notify_job_progress(size_t done, size_t total)
{
    s_job_total_cmds = static_cast<int>(total);
    s_job_done_cmds = static_cast<int>(done);
    refresh_progress_locked();
}

void ui_cnc_print_service_notify_job_end(void)
{
    s_busy = false;
    s_work_state = UI_CNC_WORK_IDLE;
    s_progress_pct = 100;
    refresh_elapsed_locked();
    s_has_eta = true;
    s_eta_sec = 0;
}

bool ui_cnc_print_service_move_to_mm_async(float x_mm, float y_mm)
{
    cnc_work_msg_t msg = {};
    msg.id = CNC_WORK_JOG;
    msg.x = x_mm;
    msg.y = y_mm;
    clamp_target_mm(&msg.x, &msg.y);
    return post_work(&msg);
}

bool ui_cnc_print_service_home_async(void)
{
    cnc_work_msg_t msg = {};
    msg.id = CNC_WORK_HOME;
    return post_work(&msg);
}

bool ui_cnc_print_service_jog_axis_mm(char axis, bool positive, float step_mm)
{
    float x = 0.0f;
    float y = 0.0f;
    ui_cnc_motion_facade_get_position_mm(&x, &y);
    const float d = positive ? step_mm : -step_mm;
    if (axis == 'X' || axis == 'x') {
        x += d;
    } else {
        y += d;
    }
    clamp_target_mm(&x, &y);
    cnc_work_msg_t msg = {};
    msg.id = CNC_WORK_JOG;
    msg.x = x;
    msg.y = y;
    return post_work(&msg);
}

bool ui_cnc_print_service_execute_gcode_file(const char *vfs_path)
{
    if (vfs_path == nullptr || vfs_path[0] == '\0') {
        return false;
    }
    cnc_work_msg_t msg = {};
    msg.id = CNC_WORK_FILE;
    strncpy(msg.file_path, vfs_path, sizeof(msg.file_path) - 1);
    msg.file_path[sizeof(msg.file_path) - 1] = '\0';
    return post_work(&msg);
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
        s_paused = false;
        s_work_state = UI_CNC_WORK_RUNNING;
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

    bind_motion_progress_callback();
    reset_job_status();

    s_work_queue = xQueueCreate(UI_CNC_WORK_QUEUE_LEN, sizeof(cnc_work_msg_t));
    if (s_work_queue == nullptr) {
        ESP_LOGE(TAG, "failed to create work queue");
        return;
    }

    if (!ensure_worker_started()) {
        ESP_LOGW(TAG, "CNC queue ready but worker task not started (retry on first command)");
    } else {
        ESP_LOGI(TAG, "CNC queue and worker ready");
    }
}
