#include "webui_api.h"
#include "webui_cnc_bridge.h"
#include "webui_log.h"
#include "webui_ws.h"

#include <cstdlib>
#include <cstring>

#include <esp_http_server.h>
#include <esp_log.h>

#if CONFIG_MSP3525_LASER_UI
extern "C" {
#include "laser_ui_state.h"
#include "ui/cnc/ui_cnc_config.h"
}
#include "ui/laser_ui_log.h"
#endif

static const char *TAG = "webui_api";

static void webui_notify_terminal(const char *msg)
{
    if (msg == nullptr) {
        return;
    }
    webui_log_append(msg);
    webui_ws_broadcast(msg);
}

static int parse_json_int(const char *body, const char *key, int default_val)
{
    if (body == nullptr || key == nullptr) {
        return default_val;
    }
    char pattern[32];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    const char *p = strstr(body, pattern);
    if (p == nullptr) {
        return default_val;
    }
    p += strlen(pattern);
    while (*p == ' ' || *p == '\t') {
        ++p;
    }
    return static_cast<int>(strtol(p, nullptr, 10));
}

static bool parse_json_bool(const char *body, const char *key, bool default_val)
{
    if (body == nullptr || key == nullptr) {
        return default_val;
    }
    char pattern[32];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    const char *p = strstr(body, pattern);
    if (p == nullptr) {
        return default_val;
    }
    p += strlen(pattern);
    while (*p == ' ' || *p == '\t') {
        ++p;
    }
    if (strncmp(p, "true", 4) == 0) {
        return true;
    }
    if (strncmp(p, "false", 5) == 0) {
        return false;
    }
    return default_val;
}

static float parse_json_float(const char *body, const char *key, float default_val)
{
    if (body == nullptr || key == nullptr) {
        return default_val;
    }
    char pattern[32];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    const char *p = strstr(body, pattern);
    if (p == nullptr) {
        return default_val;
    }
    p += strlen(pattern);
    while (*p == ' ' || *p == '\t') {
        ++p;
    }
    return strtof(p, nullptr);
}

static bool read_post_body(httpd_req_t *req, char *buf, size_t buf_size, size_t *out_len)
{
    if (req->content_len >= buf_size) {
        return false;
    }
    size_t received = 0;
    while (received < req->content_len) {
        const int r = httpd_req_recv(req, buf + received, req->content_len - received);
        if (r <= 0) {
            return false;
        }
        received += static_cast<size_t>(r);
    }
    buf[received] = '\0';
    if (out_len) {
        *out_len = received;
    }
    return true;
}

extern "C" esp_err_t webui_settings_handler(httpd_req_t *req)
{
#if !CONFIG_MSP3525_LASER_UI
    httpd_resp_send_err(req, HTTPD_503_SERVICE_UNAVAILABLE, "CNC UI disabled");
    return ESP_FAIL;
#else
    if (req->method != HTTP_POST) {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "POST only");
        return ESP_FAIL;
    }
    char body[256] = {};
    if (!read_post_body(req, body, sizeof(body), nullptr)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "body too large");
        return ESP_FAIL;
    }
    const int power = parse_json_int(body, "power_pct", -1);
    const int speed = parse_json_int(body, "speed_pct", -1);
    const int jog = parse_json_int(body, "jog_step_mm", -1);
    const bool apply_cnc = parse_json_bool(body, "apply_cnc", false);
    if (power >= 0) {
        laser_ui_state_set_power_pct(power);
    }
    if (speed >= 0) {
        laser_ui_state_set_speed_pct(speed);
    }
    if (jog > 0) {
        laser_ui_state_set_jog_step_mm(static_cast<float>(jog));
    }
    if (apply_cnc) {
        webui_cnc_apply_settings();
        webui_notify_terminal("settings applied to CNC (M3/F)\n");
    }
    const laser_ui_settings_t s = laser_ui_state_get_settings();
    const float jog_mm = laser_ui_state_get_jog_step_mm();
    ESP_LOGI(TAG, "settings applied: power=%d%% speed=%d%% jog=%.0fmm", s.laser_power_pct, s.speed_pct,
             static_cast<double>(jog_mm));
    char notify[96];
    snprintf(notify, sizeof(notify), "settings: power %d%% speed %d%% jog %.0fmm\n", s.laser_power_pct,
             s.speed_pct, static_cast<double>(jog_mm));
    webui_notify_terminal(notify);
    char resp[128];
    snprintf(resp, sizeof(resp),
             "{\"ok\":true,\"power_pct\":%d,\"speed_pct\":%d,\"jog_step_mm\":%.0f}",
             s.laser_power_pct, s.speed_pct, static_cast<double>(laser_ui_state_get_jog_step_mm()));
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, resp);
    return ESP_OK;
#endif
}

extern "C" esp_err_t webui_pick_handler(httpd_req_t *req)
{
#if !CONFIG_MSP3525_LASER_UI
    httpd_resp_send_err(req, HTTPD_503_SERVICE_UNAVAILABLE, "CNC UI disabled");
    return ESP_FAIL;
#else
    if (req->method != HTTP_POST) {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "POST only");
        return ESP_FAIL;
    }
    char body[256] = {};
    if (!read_post_body(req, body, sizeof(body), nullptr)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "body too large");
        return ESP_FAIL;
    }

    if (strstr(body, "\"reset\"") != nullptr || strstr(body, "\"action\":\"reset\"") != nullptr) {
        laser_ui_state_clear_pick_origin();
        if (!webui_cnc_home()) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "home failed");
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "pick reset (home)");
        webui_notify_terminal("pick reset: homing\n");
        httpd_resp_sendstr(req, "{\"ok\":true,\"action\":\"reset\"}");
        return ESP_OK;
    }

    float x = parse_json_float(body, "x_mm", -1.0f);
    float y = parse_json_float(body, "y_mm", -1.0f);
    if (x < 0.0f || y < 0.0f) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing x_mm/y_mm");
        return ESP_FAIL;
    }
    if (x > UI_CNC_WORK_SIZE_MM) {
        x = UI_CNC_WORK_SIZE_MM;
    }
    if (y > UI_CNC_WORK_SIZE_MM) {
        y = UI_CNC_WORK_SIZE_MM;
    }
    if (x < 0.0f) {
        x = 0.0f;
    }
    if (y < 0.0f) {
        y = 0.0f;
    }

    laser_ui_state_set_pick_origin(x, y);
    if (!webui_cnc_move_to_mm_async(x, y)) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "move failed");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "pick confirm (%.1f, %.1f)", static_cast<double>(x), static_cast<double>(y));
    char notify[64];
    snprintf(notify, sizeof(notify), "pick move to (%.1f, %.1f)\n", static_cast<double>(x),
             static_cast<double>(y));
    webui_notify_terminal(notify);
    char resp[96];
    snprintf(resp, sizeof(resp), "{\"ok\":true,\"x_mm\":%.2f,\"y_mm\":%.2f}", static_cast<double>(x),
             static_cast<double>(y));
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, resp);
    return ESP_OK;
#endif
}

extern "C" esp_err_t webui_run_handler(httpd_req_t *req)
{
#if !CONFIG_MSP3525_LASER_UI
    httpd_resp_send_err(req, HTTPD_503_SERVICE_UNAVAILABLE, "CNC UI disabled");
    return ESP_FAIL;
#else
    if (req->method != HTTP_POST) {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "POST only");
        return ESP_FAIL;
    }
    const bool was_suspended = webui_cnc_has_suspended_job();
    webui_cnc_run();
    webui_notify_terminal(was_suspended ? "run: resume job\n" : "run: laser on\n");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, was_suspended ? "{\"ok\":true,\"action\":\"resume\"}"
                                           : "{\"ok\":true,\"action\":\"run\"}");
    return ESP_OK;
#endif
}

extern "C" esp_err_t webui_pause_handler(httpd_req_t *req)
{
#if !CONFIG_MSP3525_LASER_UI
    httpd_resp_send_err(req, HTTPD_503_SERVICE_UNAVAILABLE, "CNC UI disabled");
    return ESP_FAIL;
#else
    if (req->method != HTTP_POST) {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "POST only");
        return ESP_FAIL;
    }
    webui_cnc_pause();
    webui_notify_terminal("pause: laser off, motion stop\n");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true,\"action\":\"pause\"}");
    return ESP_OK;
#endif
}

extern "C" esp_err_t webui_chat_handler(httpd_req_t *req)
{
#if !CONFIG_MSP3525_LASER_UI
    httpd_resp_send_err(req, HTTPD_503_SERVICE_UNAVAILABLE, "CNC UI disabled");
    return ESP_FAIL;
#else
    if (req->method != HTTP_GET) {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "GET only");
        return ESP_FAIL;
    }
    const char *text = laser_ui_log_get_text();
    if (text == nullptr || text[0] == '\0') {
        text = "暂无对话记录";
    }
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    httpd_resp_sendstr(req, text);
    return ESP_OK;
#endif
}
