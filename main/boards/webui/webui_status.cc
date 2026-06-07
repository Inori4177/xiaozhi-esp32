#include "webui_status.h"
#include "webui_cnc_bridge.h"

#include <cstdio>

#include <esp_http_server.h>

#if CONFIG_INTERACTION_PEER_UART
#include "peer_uart_mux.h"
#endif
#if CONFIG_INTERACTION_PEER_VOICE
#include "peer_voice_state.h"
#endif

#if CONFIG_MSP3525_LASER_UI
extern "C" {
#include "laser_ui_state.h"
#include "ui/cnc/ui_cnc_config.h"
#include "ui/cnc/ui_cnc_print_service.h"
}
#endif

static const char *work_state_text(int state)
{
#if CONFIG_MSP3525_LASER_UI
    switch (static_cast<ui_cnc_work_state_t>(state)) {
    case UI_CNC_WORK_RUNNING:
        return "雕刻中";
    case UI_CNC_WORK_JOGGING:
        return "移动中";
    case UI_CNC_WORK_PAUSED:
        return "暂停";
    case UI_CNC_WORK_IDLE:
    default:
        return "空闲";
    }
#else
    (void)state;
    return "空闲";
#endif
}

extern "C" esp_err_t webui_status_handler(httpd_req_t *req)
{
    if (req->method != HTTP_GET) {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "GET only");
        return ESP_FAIL;
    }

    char json[768] = {};
    float x = 0.0f;
    float y = 0.0f;
    webui_cnc_get_position(&x, &y);

#if CONFIG_MSP3525_LASER_UI
    ui_cnc_print_status_t st = {};
    ui_cnc_print_service_get_status(&st);
    const laser_ui_settings_t settings = laser_ui_state_get_settings();
    float ox = 0.0f;
    float oy = 0.0f;
    const bool has_origin = laser_ui_state_get_pick_origin(&ox, &oy);
    const bool peer_ready = webui_cnc_worker_ready();
#if CONFIG_INTERACTION_PEER_UART
    const bool uart_up = peer_uart_link_is_up();
#else
    const bool uart_up = false;
#endif
#if CONFIG_INTERACTION_PEER_VOICE
    const char *voice_state = peer_voice_state_get();
#else
    const char *voice_state = "idle";
#endif
    snprintf(json, sizeof(json),
             "{\"state\":\"%s\",\"state_id\":%d,\"progress\":%u,\"elapsed_sec\":%u,"
             "\"eta_sec\":%u,\"has_eta\":%s,\"file\":\"%s\",\"x_mm\":%.2f,\"y_mm\":%.2f,"
             "\"work_mm\":%.1f,\"jog_step_mm\":%.1f,\"power_pct\":%d,\"speed_pct\":%d,"
             "\"pick_x\":%.2f,\"pick_y\":%.2f,\"pick_valid\":%s,\"busy\":%s,"
             "\"peer_ready\":%s,\"uart_up\":%s,\"voice_state\":\"%s\"}",
             work_state_text(static_cast<int>(st.state)), static_cast<int>(st.state),
             static_cast<unsigned>(st.progress_pct), static_cast<unsigned>(st.elapsed_sec),
             static_cast<unsigned>(st.eta_sec), st.has_eta ? "true" : "false",
             ui_cnc_print_service_get_job_name(), static_cast<double>(x), static_cast<double>(y),
             static_cast<double>(UI_CNC_WORK_SIZE_MM),
             static_cast<double>(laser_ui_state_get_jog_step_mm()), settings.laser_power_pct,
             settings.speed_pct, static_cast<double>(ox), static_cast<double>(oy),
             has_origin ? "true" : "false", webui_cnc_is_busy() ? "true" : "false",
             peer_ready ? "true" : "false", uart_up ? "true" : "false", voice_state);
#else
    snprintf(json, sizeof(json),
             "{\"state\":\"空闲\",\"state_id\":0,\"progress\":0,\"elapsed_sec\":0,"
             "\"eta_sec\":0,\"has_eta\":false,\"file\":\"-\",\"x_mm\":%.2f,\"y_mm\":%.2f,"
             "\"work_mm\":42,\"jog_step_mm\":1,\"power_pct\":50,\"speed_pct\":100,"
             "\"pick_x\":0,\"pick_y\":0,\"pick_valid\":false,\"busy\":false,"
             "\"peer_ready\":false,\"uart_up\":false,\"voice_state\":\"idle\"}",
             static_cast<double>(x), static_cast<double>(y));
#endif

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json);
    return ESP_OK;
}
