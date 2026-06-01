/**
 * CNC print service — forwards to motion ESP32-S3 over UART when INTERACTION_PEER_UART=y.
 */
#include "ui_cnc_print_service.h"

#include "../laser_ui_events.h"
#include "../../laser_ui_state.h"

#if CONFIG_INTERACTION_PEER_UART
#include "../../peer_link/peer_cnc_client.h"
#endif

#include <esp_log.h>
#include <string.h>

static const char *TAG = "ui_cnc_svc";

#if !CONFIG_INTERACTION_PEER_UART

static ui_cnc_print_status_t s_status = {
    .state = UI_CNC_WORK_IDLE,
    .progress_pct = 0,
    .elapsed_sec = 0,
    .eta_sec = 0,
    .has_eta = false,
    .is_print_job = false,
};

#endif

void ui_cnc_print_service_init(void)
{
#if CONFIG_INTERACTION_PEER_UART
    if (!peer_cnc_client_init()) {
        ESP_LOGE(TAG, "peer CNC client init failed");
    } else {
        ESP_LOGI(TAG, "CNC via UART peer link");
    }
#else
    ESP_LOGW(TAG, "CNC stub (no peer UART)");
#endif
}

bool ui_cnc_print_service_execute_gcode(const char *gcode_text)
{
    if (gcode_text == nullptr || gcode_text[0] == '\0') {
        return true;
    }
#if CONFIG_INTERACTION_PEER_UART
    return peer_cnc_client_send_gcode(gcode_text);
#else
    (void)gcode_text;
    return true;
#endif
}

bool ui_cnc_print_service_move_to_mm_async(float x_mm, float y_mm)
{
#if CONFIG_INTERACTION_PEER_UART
    return peer_cnc_client_move_to_mm_async(x_mm, y_mm);
#else
    (void)x_mm;
    (void)y_mm;
    return true;
#endif
}

bool ui_cnc_print_service_home_async(void)
{
#if CONFIG_INTERACTION_PEER_UART
    return peer_cnc_client_home_async();
#else
    return true;
#endif
}

bool ui_cnc_print_service_jog_axis_mm(char axis, bool positive, float step_mm)
{
#if CONFIG_INTERACTION_PEER_UART
    return peer_cnc_client_jog_axis_mm(axis, positive, step_mm);
#else
    (void)axis;
    (void)positive;
    (void)step_mm;
    return true;
#endif
}

bool ui_cnc_print_service_execute_gcode_file(const char *vfs_path)
{
#if CONFIG_INTERACTION_PEER_UART
    return peer_cnc_client_execute_gcode_file(vfs_path);
#else
    (void)vfs_path;
    return false;
#endif
}

bool ui_cnc_print_service_worker_ready(void)
{
#if CONFIG_INTERACTION_PEER_UART
    return peer_cnc_client_is_ready();
#else
    return true;
#endif
}

bool ui_cnc_print_service_is_busy(void)
{
#if CONFIG_INTERACTION_PEER_UART
    return peer_cnc_client_is_busy();
#else
    return false;
#endif
}

const char *ui_cnc_print_service_get_job_name(void)
{
#if CONFIG_INTERACTION_PEER_UART
    return peer_cnc_client_get_job_name();
#else
    return "-";
#endif
}

void ui_cnc_print_service_get_status(ui_cnc_print_status_t *out)
{
#if CONFIG_INTERACTION_PEER_UART
    peer_cnc_client_get_status(out);
#else
    if (out != nullptr) {
        *out = s_status;
    }
#endif
}

void ui_cnc_print_service_get_position_mm(float *x_mm, float *y_mm)
{
#if CONFIG_INTERACTION_PEER_UART
    peer_cnc_client_get_position_mm(x_mm, y_mm);
#else
    if (x_mm != nullptr) {
        *x_mm = 0.0f;
    }
    if (y_mm != nullptr) {
        *y_mm = 0.0f;
    }
#endif
}

void ui_cnc_print_service_notify_job_begin(const char *gcode_text)
{
    (void)gcode_text;
}

void ui_cnc_print_service_notify_job_progress(size_t done, size_t total)
{
    (void)done;
    (void)total;
}

void ui_cnc_print_service_notify_job_end(void)
{
}

void ui_cnc_print_service_on_event(laser_ui_event_id_t id)
{
#if CONFIG_INTERACTION_PEER_UART
    peer_cnc_client_on_ui_event(id);
#else
    switch (id) {
    case LASER_EVT_RUN:
        ui_cnc_print_service_run();
        break;
    case LASER_EVT_PAUSE:
        ui_cnc_print_service_pause();
        break;
    case LASER_EVT_SETTINGS_APPLY:
        ui_cnc_print_service_apply_settings();
        break;
    default:
        break;
    }
#endif
}

void ui_cnc_print_service_pause(void)
{
#if CONFIG_INTERACTION_PEER_UART
    peer_cnc_client_pause();
#endif
}

void ui_cnc_print_service_run(void)
{
#if CONFIG_INTERACTION_PEER_UART
    peer_cnc_client_run();
#endif
}

void ui_cnc_print_service_apply_settings(void)
{
#if CONFIG_INTERACTION_PEER_UART
    peer_cnc_client_apply_settings();
#endif
}

bool ui_cnc_print_service_has_suspended_job(void)
{
#if CONFIG_INTERACTION_PEER_UART
    return peer_cnc_client_has_suspended_job();
#else
    return false;
#endif
}

bool ui_cnc_print_service_poll_pause_abort(void)
{
    return false;
}
