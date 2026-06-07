#include "peer_cnc_slave.h"

#include "motion_controller.h"
#include "stepper.h"
#include "peer_uart_link.h"

#include <cJSON.h>
#include <cstdio>
#include <cstring>
#include <string>

#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

static const char *TAG = "peer_cnc_slave";

enum slave_cmd_t : uint8_t {
    SLAVE_CMD_GCODE = 0,
    SLAVE_CMD_JOG,
    SLAVE_CMD_HOME,
    SLAVE_CMD_MOVE,
    SLAVE_CMD_PAUSE,
    SLAVE_CMD_RUN,
    SLAVE_CMD_APPLY,
};

struct slave_msg_t {
    slave_cmd_t type;
    char line[256];
    float f0;
    float f1;
    int i0;
    int i1;
    char axis;
};

static QueueHandle_t s_cmd_q = nullptr;
static TaskHandle_t s_worker = nullptr;
static float s_pos_x = 0.0f;
static float s_pos_y = 0.0f;
static int s_work_state = 0;
static uint8_t s_progress = 0;
static bool s_busy = false;
static bool s_paused = false;
static char s_job_name[64] = "-";
static int s_power_pct = 50;
static int s_speed_pct = 100;
static uint32_t s_file_lines = 0;
static uint32_t s_file_done = 0;

static bool send_json(const char *json)
{
    return peer_uart_link_send_line(json);
}

static void send_pong(void)
{
    send_json("{\"t\":\"pong\",\"ok\":true}");
}

static void send_ack(bool ok, const char *err)
{
    if (ok) {
        send_json("{\"t\":\"ack\",\"ok\":true}");
    } else {
        char buf[128];
        snprintf(buf, sizeof(buf), "{\"t\":\"ack\",\"ok\":false,\"err\":\"%s\"}", err ? err : "fail");
        send_json(buf);
    }
}

static void send_log(const char *msg)
{
    char buf[384];
    snprintf(buf, sizeof(buf), "{\"t\":\"log\",\"msg\":\"%s\"}", msg ? msg : "");
    send_json(buf);
}

static void push_status_pos(void)
{
    char status[256];
    snprintf(status, sizeof(status),
             "{\"t\":\"status\",\"state\":%d,\"pct\":%u,\"elapsed\":0,\"eta\":0,"
             "\"has_eta\":false,\"busy\":%s,\"file\":\"%s\"}",
             s_work_state, static_cast<unsigned>(s_progress), s_busy ? "true" : "false", s_job_name);
    send_json(status);

    char pos[96];
    snprintf(pos, sizeof(pos), "{\"t\":\"pos\",\"x\":%.2f,\"y\":%.2f}",
             static_cast<double>(s_pos_x), static_cast<double>(s_pos_y));
    send_json(pos);
}

static bool enqueue_msg(const slave_msg_t &msg)
{
    if (s_cmd_q == nullptr) {
        return false;
    }
    return xQueueSend(s_cmd_q, &msg, pdMS_TO_TICKS(50)) == pdTRUE;
}

static void run_gcode_line(const char *line)
{
    if (line == nullptr || line[0] == '\0' || s_paused) {
        return;
    }
    s_work_state = 1;
    s_busy = true;
    std::string gcode(line);
    gcode.push_back('\n');
    MotionController::Get().Execute(gcode, false);
    s_pos_x = MotionController::Get().GetX();
    s_pos_y = MotionController::Get().GetY();
    s_file_done++;
    if (s_file_lines > 0) {
        s_progress = static_cast<uint8_t>((s_file_done * 100U) / s_file_lines);
    }
    send_log(line);
}

static void worker_task(void *arg)
{
    (void)arg;
    slave_msg_t msg = {};
    for (;;) {
        if (xQueueReceive(s_cmd_q, &msg, pdMS_TO_TICKS(200)) != pdTRUE) {
            continue;
        }
        switch (msg.type) {
        case SLAVE_CMD_GCODE:
            run_gcode_line(msg.line);
            break;
        case SLAVE_CMD_JOG: {
            const float delta = msg.f0 * static_cast<float>(msg.i0);
            char gcode[64];
            if (msg.axis == 'X' || msg.axis == 'x') {
                s_pos_x += delta;
                snprintf(gcode, sizeof(gcode), "G0 X%.3f F700", static_cast<double>(s_pos_x));
            } else {
                s_pos_y += delta;
                snprintf(gcode, sizeof(gcode), "G0 Y%.3f F700", static_cast<double>(s_pos_y));
            }
            run_gcode_line(gcode);
            break;
        }
        case SLAVE_CMD_HOME:
            s_work_state = 2;
            MotionController::Get().Execute("G0 X0 Y0 F700\n", false);
            s_pos_x = 0.0f;
            s_pos_y = 0.0f;
            s_work_state = 0;
            s_busy = false;
            send_log("home done");
            break;
        case SLAVE_CMD_MOVE:
            s_work_state = 2;
            {
                char gcode[64];
                snprintf(gcode, sizeof(gcode), "G0 X%.3f Y%.3f F700",
                         static_cast<double>(msg.f0), static_cast<double>(msg.f1));
                run_gcode_line(gcode);
            }
            s_work_state = 0;
            s_busy = false;
            break;
        case SLAVE_CMD_PAUSE:
            s_paused = true;
            s_work_state = 3;
            Stepper::GoIdle();
            send_log("paused");
            break;
        case SLAVE_CMD_RUN:
            s_paused = false;
            s_work_state = s_busy ? 1 : 0;
            send_log("run");
            break;
        case SLAVE_CMD_APPLY:
            s_power_pct = msg.i0;
            s_speed_pct = msg.i1;
            send_log("settings applied");
            break;
        default:
            break;
        }
        push_status_pos();
    }
}

static void status_task(void *arg)
{
    (void)arg;
    for (;;) {
        push_status_pos();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void on_host_line(const char *line, void *user_data)
{
    (void)user_data;
    if (line == nullptr || line[0] == '\0') {
        return;
    }

    cJSON *root = cJSON_Parse(line);
    if (root == nullptr) {
        return;
    }

    const cJSON *type = cJSON_GetObjectItem(root, "t");
    const char *t = cJSON_IsString(type) ? type->valuestring : nullptr;
    slave_msg_t msg = {};

    if (t == nullptr) {
        cJSON_Delete(root);
        return;
    }

    ESP_LOGI(TAG, "cmd: %s", t);

    if (strcmp(t, "ping") == 0) {
        send_pong();
    } else if (strcmp(t, "poll") == 0) {
        push_status_pos();
    } else if (strcmp(t, "gcode") == 0) {
        const cJSON *gline = cJSON_GetObjectItem(root, "line");
        if (cJSON_IsString(gline)) {
            msg.type = SLAVE_CMD_GCODE;
            snprintf(msg.line, sizeof(msg.line), "%s", gline->valuestring);
            enqueue_msg(msg);
        }
    } else if (strcmp(t, "jog") == 0) {
        const cJSON *axis = cJSON_GetObjectItem(root, "axis");
        const cJSON *step = cJSON_GetObjectItem(root, "step");
        const cJSON *sign = cJSON_GetObjectItem(root, "sign");
        if (cJSON_IsString(axis) && cJSON_IsNumber(step) && cJSON_IsNumber(sign)) {
            msg.type = SLAVE_CMD_JOG;
            msg.f0 = static_cast<float>(step->valuedouble);
            msg.i0 = sign->valueint;
            msg.axis = axis->valuestring[0];
            enqueue_msg(msg);
        }
    } else if (strcmp(t, "home") == 0) {
        msg.type = SLAVE_CMD_HOME;
        enqueue_msg(msg);
    } else if (strcmp(t, "move") == 0) {
        const cJSON *x = cJSON_GetObjectItem(root, "x");
        const cJSON *y = cJSON_GetObjectItem(root, "y");
        if (cJSON_IsNumber(x) && cJSON_IsNumber(y)) {
            msg.type = SLAVE_CMD_MOVE;
            msg.f0 = static_cast<float>(x->valuedouble);
            msg.f1 = static_cast<float>(y->valuedouble);
            enqueue_msg(msg);
        }
    } else if (strcmp(t, "pause") == 0) {
        msg.type = SLAVE_CMD_PAUSE;
        enqueue_msg(msg);
    } else if (strcmp(t, "run") == 0) {
        msg.type = SLAVE_CMD_RUN;
        enqueue_msg(msg);
    } else if (strcmp(t, "apply") == 0) {
        const cJSON *power = cJSON_GetObjectItem(root, "power");
        const cJSON *speed = cJSON_GetObjectItem(root, "speed");
        msg.type = SLAVE_CMD_APPLY;
        msg.i0 = cJSON_IsNumber(power) ? power->valueint : s_power_pct;
        msg.i1 = cJSON_IsNumber(speed) ? speed->valueint : s_speed_pct;
        enqueue_msg(msg);
    } else if (strcmp(t, "file_begin") == 0) {
        const cJSON *name = cJSON_GetObjectItem(root, "name");
        if (cJSON_IsString(name)) {
            snprintf(s_job_name, sizeof(s_job_name), "%s", name->valuestring);
        }
        s_file_lines = 0;
        s_file_done = 0;
        s_progress = 0;
        s_busy = true;
        s_work_state = 1;
        push_status_pos();
    } else if (strcmp(t, "file_end") == 0) {
        s_busy = false;
        s_work_state = 0;
        s_progress = 100;
        push_status_pos();
    } else {
        ESP_LOGW(TAG, "unknown cmd: %s", t);
    }

    cJSON_Delete(root);
}

bool peer_cnc_slave_init(void)
{
    MotionController::GlobalInit(106.666f, 106.666f, 700.0f, 800.0f, 0.02f, 42.0f);

    if (!peer_uart_link_init()) {
        ESP_LOGE(TAG, "UART init failed");
        return false;
    }
    peer_uart_link_set_line_callback(on_host_line, nullptr);

    if (s_cmd_q == nullptr) {
        s_cmd_q = xQueueCreate(16, sizeof(slave_msg_t));
    }
    if (s_worker == nullptr) {
        xTaskCreatePinnedToCore(worker_task, "peer_cnc_worker", 8192, nullptr, 4, &s_worker, 1);
    }
    xTaskCreatePinnedToCore(status_task, "peer_cnc_stat", 3072, nullptr, 3, nullptr, 0);

    ESP_LOGI(TAG, "peer CNC slave ready");
    return true;
}
