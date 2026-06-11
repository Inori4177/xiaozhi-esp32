#include "peer_cnc_client.h"
#include "peer_uart_link.h"

#include "../laser_ui_state.h"
#include "../ui/gcode/ui_gcode_preview.h"
#if CONFIG_MSP3525_LASER_UI
#include "../ui/pick/ui_pick_map_preview.h"
#endif
#include "boards/webui/webui_config.h"
#include "boards/webui/webui_log.h"
#include "boards/webui/webui_ws.h"

#include <cJSON.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "peer_cnc";

static ui_cnc_print_status_t s_status = {
    .state = UI_CNC_WORK_IDLE,
    .progress_pct = 0,
    .elapsed_sec = 0,
    .eta_sec = 0,
    .has_eta = false,
    .is_print_job = false,
};

static float s_pos_x = 0.0f;
static float s_pos_y = 0.0f;
static char s_job_name[64] = "-";
static bool s_peer_ready = false;
static bool s_has_suspended = false;
static bool s_local_file_job = false;
static bool s_await_cnc_idle = false;
static int64_t s_job_start_us = 0;
static TaskHandle_t s_file_task = nullptr;

static void mark_peer_alive(void)
{
    s_peer_ready = true;
}

static bool send_json(const char *json)
{
    if (json == nullptr) {
        return false;
    }
    const bool ok = peer_uart_link_send_line(json);
    if (ok) {
        ESP_LOGI(TAG, "TX %s (peer_ready=%d)", json, s_peer_ready ? 1 : 0);
    } else {
        ESP_LOGW(TAG, "TX failed: %s", json);
    }
    return ok;
}

static bool send_cmd_gcode(const char *line)
{
    if (line == nullptr) {
        return false;
    }
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "t", "gcode");
    cJSON_AddStringToObject(root, "line", line);
    char *js = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (js == nullptr) {
        return false;
    }
    const bool ok = send_json(js);
    cJSON_free(js);
    return ok;
}

static void refresh_local_elapsed(void)
{
    if (!s_status.is_print_job || s_job_start_us <= 0) {
        return;
    }
    const int64_t now = esp_timer_get_time();
    if (now <= s_job_start_us) {
        s_status.elapsed_sec = 0;
        return;
    }
    s_status.elapsed_sec = static_cast<uint32_t>((now - s_job_start_us) / 1000000LL);
    if (s_status.progress_pct > 0 && s_status.progress_pct < 100) {
        const uint32_t total_est =
            (s_status.elapsed_sec * 100U + static_cast<uint32_t>(s_status.progress_pct) - 1U) /
            static_cast<uint32_t>(s_status.progress_pct);
        if (total_est > s_status.elapsed_sec) {
            s_status.eta_sec = total_est - s_status.elapsed_sec;
            s_status.has_eta = true;
        } else {
            s_status.has_eta = false;
        }
    } else {
        s_status.has_eta = false;
    }
}

static void begin_local_file_job(const char *base_name, size_t line_count)
{
    if (base_name != nullptr) {
        snprintf(s_job_name, sizeof(s_job_name), "%s", base_name);
    }
    s_has_suspended = false;
    s_local_file_job = true;
    s_status.state = UI_CNC_WORK_RUNNING;
    s_status.is_print_job = true;
    s_status.progress_pct = 0;
    s_status.elapsed_sec = 0;
    s_status.eta_sec = 0;
    s_status.has_eta = false;
    s_job_start_us = esp_timer_get_time();
    ESP_LOGI(TAG, "file job begin: %s (%u lines)", s_job_name, static_cast<unsigned>(line_count));
}

static void end_local_file_job(void)
{
    s_status.state = UI_CNC_WORK_IDLE;
    s_status.is_print_job = false;
    s_status.progress_pct = 0;
    s_status.elapsed_sec = 0;
    s_status.eta_sec = 0;
    s_status.has_eta = false;
    s_local_file_job = false;
    s_await_cnc_idle = false;
    s_job_start_us = 0;
    ESP_LOGI(TAG, "file job end: %s", s_job_name);
}

static void try_finish_file_job_from_peer(const cJSON *root)
{
    if (!s_local_file_job || !s_await_cnc_idle) {
        return;
    }
    const cJSON *state = cJSON_GetObjectItem(root, "state");
    const cJSON *busy = cJSON_GetObjectItem(root, "busy");
    const int st = cJSON_IsNumber(state) ? state->valueint : -1;
    const bool peer_busy = cJSON_IsBool(busy) && cJSON_IsTrue(busy);
    if (st == static_cast<int>(UI_CNC_WORK_IDLE) && !peer_busy) {
        end_local_file_job();
    }
}

static void parse_status(const cJSON *root)
{
    const cJSON *state = cJSON_GetObjectItem(root, "state");
    const cJSON *pct = cJSON_GetObjectItem(root, "pct");
    const cJSON *elapsed = cJSON_GetObjectItem(root, "elapsed");
    const cJSON *eta = cJSON_GetObjectItem(root, "eta");
    const cJSON *has_eta = cJSON_GetObjectItem(root, "has_eta");
    const cJSON *busy = cJSON_GetObjectItem(root, "busy");
    const cJSON *file = cJSON_GetObjectItem(root, "file");

    if (s_local_file_job && s_await_cnc_idle) {
        /* Lines already sent (progress_pct=100). Peer pct tracks motion execution
         * lag and would briefly show ~1–5% until the queue drains — ignore it. */
        if (cJSON_IsNumber(state)) {
            s_status.state = static_cast<ui_cnc_work_state_t>(state->valueint);
        }
        if (cJSON_IsBool(busy)) {
            s_status.is_print_job = cJSON_IsTrue(busy);
        }
        try_finish_file_job_from_peer(root);
        return;
    }

    if (s_local_file_job) {
        if (cJSON_IsNumber(state)) {
            const auto st = static_cast<ui_cnc_work_state_t>(state->valueint);
            if (st == UI_CNC_WORK_PAUSED || st == UI_CNC_WORK_RUNNING) {
                s_status.state = st;
            }
        }
        return;
    }

    if (cJSON_IsNumber(state)) {
        s_status.state = static_cast<ui_cnc_work_state_t>(state->valueint);
    }
    if (cJSON_IsNumber(pct)) {
        s_status.progress_pct = static_cast<uint8_t>(pct->valueint);
    }
    if (cJSON_IsNumber(elapsed)) {
        s_status.elapsed_sec = static_cast<uint32_t>(elapsed->valueint);
    }
    if (cJSON_IsNumber(eta)) {
        s_status.eta_sec = static_cast<uint32_t>(eta->valueint);
    }
    if (cJSON_IsBool(has_eta)) {
        s_status.has_eta = cJSON_IsTrue(has_eta);
    }
    if (cJSON_IsBool(busy)) {
        s_status.is_print_job = cJSON_IsTrue(busy);
    }
    if (cJSON_IsString(file) && file->valuestring != nullptr) {
        snprintf(s_job_name, sizeof(s_job_name), "%s", file->valuestring);
    }
}

static void on_peer_line(const char *line, void *user_data)
{
    (void)user_data;
    if (line == nullptr || line[0] == '\0') {
        return;
    }
    ESP_LOGD(TAG, "RX %s", line);

    cJSON *root = cJSON_Parse(line);
    if (root == nullptr) {
        ESP_LOGW(TAG, "bad JSON");
        return;
    }

    const cJSON *type = cJSON_GetObjectItem(root, "t");
    const char *t = cJSON_IsString(type) ? type->valuestring : nullptr;

    if (t != nullptr && strcmp(t, "pong") == 0 && !s_peer_ready) {
        ESP_LOGI(TAG, "peer link ready (pong)");
    }
    mark_peer_alive();

    if (t != nullptr && strcmp(t, "status") == 0) {
        parse_status(root);
    } else if (t != nullptr && strcmp(t, "pos") == 0) {
        const cJSON *x = cJSON_GetObjectItem(root, "x");
        const cJSON *y = cJSON_GetObjectItem(root, "y");
        if (cJSON_IsNumber(x)) {
            s_pos_x = static_cast<float>(x->valuedouble);
        }
        if (cJSON_IsNumber(y)) {
            s_pos_y = static_cast<float>(y->valuedouble);
        }
    } else if (t != nullptr && strcmp(t, "log") == 0) {
        const cJSON *msg = cJSON_GetObjectItem(root, "msg");
        if (cJSON_IsString(msg) && msg->valuestring != nullptr) {
            webui_log_append(msg->valuestring);
        }
    } else if (t != nullptr && strcmp(t, "ack") == 0) {
        const cJSON *ok = cJSON_GetObjectItem(root, "ok");
        if (cJSON_IsBool(ok) && !cJSON_IsTrue(ok)) {
            const cJSON *err = cJSON_GetObjectItem(root, "err");
            if (cJSON_IsString(err)) {
                ESP_LOGW(TAG, "peer nack: %s", err->valuestring);
            }
        }
    }

    cJSON_Delete(root);
}

static bool read_gcode_lines(const char *path, std::vector<std::string> &lines_out)
{
    FILE *f = fopen(path, "rb");
    if (f == nullptr) {
        return false;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return false;
    }
    const long file_size = ftell(f);
    if (file_size < 0 || file_size > static_cast<long>(WEBUI_GCODEGEN_MAX_BYTES)) {
        fclose(f);
        return false;
    }
    rewind(f);

    char line[256];
    while (fgets(line, sizeof(line), f) != nullptr) {
        size_t n = strlen(line);
        while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r')) {
            line[--n] = '\0';
        }
        if (n == 0 || line[0] == ';') {
            continue;
        }
        lines_out.emplace_back(line);
    }
    fclose(f);
    return !lines_out.empty();
}

static void file_stream_task(void *arg)
{
    char *path = static_cast<char *>(arg);
    if (path == nullptr) {
        vTaskDelete(nullptr);
        return;
    }

    std::vector<std::string> lines;
    if (!read_gcode_lines(path, lines)) {
        ESP_LOGE(TAG, "read failed: %s", path);
        free(path);
        s_file_task = nullptr;
        vTaskDelete(nullptr);
        return;
    }

    const char *base = strrchr(path, '/');
    base = (base != nullptr) ? base + 1 : path;
    begin_local_file_job(base, lines.size());

    char begin[160];
    snprintf(begin, sizeof(begin), "{\"t\":\"file_begin\",\"name\":\"%s\",\"lines\":%u}", base,
             static_cast<unsigned>(lines.size()));
    send_json(begin);

    float origin_x = 0.0f;
    float origin_y = 0.0f;
    const bool has_origin = laser_ui_state_get_pick_origin(&origin_x, &origin_y);
    bool abs_mode = true;

    for (size_t i = 0; i < lines.size(); ++i) {
        while (s_has_suspended) {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        char line_buf[256];
        strncpy(line_buf, lines[i].c_str(), sizeof(line_buf) - 1);
        line_buf[sizeof(line_buf) - 1] = '\0';
        if (has_origin) {
            ui_gcode_preview_offset_gcode_line(line_buf, sizeof(line_buf), &abs_mode, origin_x,
                                             origin_y);
        } else if (strstr(line_buf, "G90") != nullptr) {
            abs_mode = true;
        } else if (strstr(line_buf, "G91") != nullptr) {
            abs_mode = false;
        }
        send_cmd_gcode(line_buf);
        s_status.progress_pct =
            static_cast<uint8_t>(((i + 1U) * 100U) / static_cast<unsigned>(lines.size()));
        refresh_local_elapsed();
        vTaskDelay(1);
    }

    send_json("{\"t\":\"file_end\"}");
    s_await_cnc_idle = true;
    s_status.progress_pct = 100;
    refresh_local_elapsed();
    free(path);
    s_file_task = nullptr;
    vTaskDelete(nullptr);
}

bool peer_cnc_client_init(void)
{
    if (!peer_uart_link_init()) {
        return false;
    }
    peer_uart_link_set_line_callback(on_peer_line, nullptr);
    send_json("{\"t\":\"ping\"}");
    return true;
}

bool peer_cnc_client_is_ready(void)
{
    return s_peer_ready && peer_uart_link_is_up();
}

bool peer_cnc_client_send_gcode(const char *line)
{
    return send_cmd_gcode(line);
}

bool peer_cnc_client_jog_axis_mm(char axis, bool positive, float step_mm)
{
    char buf[128];
    snprintf(buf, sizeof(buf),
             "{\"t\":\"jog\",\"axis\":\"%c\",\"step\":%.3f,\"sign\":%d}",
             axis, static_cast<double>(step_mm), positive ? 1 : -1);
    return send_json(buf);
}

bool peer_cnc_client_home_async(void)
{
    return send_json("{\"t\":\"home\"}");
}

bool peer_cnc_client_move_to_mm_async(float x_mm, float y_mm)
{
    char buf[96];
    snprintf(buf, sizeof(buf), "{\"t\":\"move\",\"x\":%.3f,\"y\":%.3f}",
             static_cast<double>(x_mm), static_cast<double>(y_mm));
    return send_json(buf);
}

bool peer_cnc_client_execute_gcode_file(const char *vfs_path)
{
    if (vfs_path == nullptr || s_file_task != nullptr) {
        return false;
    }
    s_has_suspended = false;
    send_json("{\"t\":\"run\"}");
    char *copy = strdup(vfs_path);
    if (copy == nullptr) {
        return false;
    }
    if (xTaskCreatePinnedToCore(file_stream_task, "peer_file", 8192, copy, 4, &s_file_task, 0) != pdPASS) {
        free(copy);
        return false;
    }
    return true;
}

bool peer_cnc_client_pause(void)
{
    s_has_suspended = true;
    if (s_local_file_job) {
        s_status.state = UI_CNC_WORK_PAUSED;
        s_status.is_print_job = true;
    }
    return send_json("{\"t\":\"pause\"}");
}

bool peer_cnc_client_run(void)
{
    s_has_suspended = false;
    if (s_local_file_job && s_status.state == UI_CNC_WORK_PAUSED) {
        s_status.state = UI_CNC_WORK_RUNNING;
    }
    return send_json("{\"t\":\"run\"}");
}

bool peer_cnc_client_apply_settings(void)
{
    const laser_ui_settings_t st = laser_ui_state_get_settings();
    char buf[96];
    snprintf(buf, sizeof(buf), "{\"t\":\"apply\",\"power\":%d,\"speed\":%d}",
             st.laser_power_pct, st.speed_pct);
    return send_json(buf);
}

void peer_cnc_client_get_status(ui_cnc_print_status_t *out)
{
    if (out == nullptr) {
        return;
    }
    refresh_local_elapsed();
    *out = s_status;
}

void peer_cnc_client_get_position_mm(float *x_mm, float *y_mm)
{
    if (x_mm) {
        *x_mm = s_pos_x;
    }
    if (y_mm) {
        *y_mm = s_pos_y;
    }
}

bool peer_cnc_client_is_busy(void)
{
    return s_file_task != nullptr || s_local_file_job ||
           s_status.state == UI_CNC_WORK_RUNNING || s_status.state == UI_CNC_WORK_JOGGING;
}

const char *peer_cnc_client_get_job_name(void)
{
    return s_job_name;
}

bool peer_cnc_client_has_suspended_job(void)
{
    return s_has_suspended;
}

void peer_cnc_client_on_ui_event(laser_ui_event_id_t id)
{
    const float step = laser_ui_state_get_jog_step_mm();
    ESP_LOGI(TAG, "ui event: %s (jog_step=%.1fmm, peer_ready=%d)",
             laser_ui_event_name(id), static_cast<double>(step), s_peer_ready ? 1 : 0);
    switch (id) {
    case LASER_EVT_JOG_X_PLUS:
        peer_cnc_client_jog_axis_mm('X', true, step);
        break;
    case LASER_EVT_JOG_X_MINUS:
        peer_cnc_client_jog_axis_mm('X', false, step);
        break;
    case LASER_EVT_JOG_Y_PLUS:
        peer_cnc_client_jog_axis_mm('Y', true, step);
        break;
    case LASER_EVT_JOG_Y_MINUS:
        peer_cnc_client_jog_axis_mm('Y', false, step);
        break;
    case LASER_EVT_JOG_HOME:
        peer_cnc_client_home_async();
        break;
    case LASER_EVT_RUN:
        peer_cnc_client_run();
        break;
    case LASER_EVT_PAUSE:
        peer_cnc_client_pause();
        break;
    case LASER_EVT_SETTINGS_APPLY:
        peer_cnc_client_apply_settings();
#if CONFIG_MSP3525_LASER_UI
        ui_pick_map_preview_refresh_colors();
#endif
        break;
    default:
        ESP_LOGW(TAG, "ui event not handled by peer: %s", laser_ui_event_name(id));
        break;
    }
}
