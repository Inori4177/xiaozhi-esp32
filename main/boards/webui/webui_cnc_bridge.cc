#include "webui_cnc_bridge.h"
#include "webui_log.h"

#include <cstdio>
#include <cmath>
#include <cstring>
#include <string>

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#if CONFIG_MSP3525_LASER_UI
extern "C" {
#include "ui/cnc/ui_cnc_print_service.h"
#include "ui/cnc/ui_cnc_motion_facade.h"
}
#include "laser_ui_state.h"
#endif

static const char *TAG = "webui_cnc";

#if CONFIG_MSP3525_LASER_UI

bool webui_cnc_available(void)
{
    return ui_cnc_print_service_worker_ready();
}

bool webui_cnc_worker_ready(void)
{
    return ui_cnc_print_service_worker_ready();
}

bool webui_cnc_submit_line(const char *line)
{
    if (line == nullptr || line[0] == '\0') {
        return true;
    }
    const bool ok = ui_cnc_print_service_execute_gcode(line);
    webui_log_appendf("> %s\n", line);
    return ok;
}

bool webui_cnc_jog(const char *axis_delta, float feed_mm_min)
{
    (void)feed_mm_min;
    if (axis_delta == nullptr || axis_delta[0] == '\0') {
        return false;
    }
    char axis = axis_delta[0];
    float delta = 0.0f;
    if (strlen(axis_delta) > 1) {
        delta = strtof(axis_delta + 1, nullptr);
    }
    if (delta == 0.0f) {
        delta = laser_ui_state_get_jog_step_mm();
    }
    const bool positive = (delta >= 0.0f);
    const float step = fabsf(delta);
    const bool ok = ui_cnc_print_service_jog_axis_mm(axis, positive, step);
    webui_log_appendf("jog %c %s%.3f\n", axis, positive ? "+" : "-", static_cast<double>(step));
    return ok;
}

bool webui_cnc_home(void)
{
    const bool ok = ui_cnc_print_service_home_async();
    webui_log_append("home\n");
    return ok;
}

bool webui_cnc_move_to_mm_async(float x_mm, float y_mm)
{
    return ui_cnc_print_service_move_to_mm_async(x_mm, y_mm);
}

bool webui_cnc_run_file(const char *vfs_path)
{
    if (vfs_path == nullptr || vfs_path[0] == '\0') {
        return false;
    }
    FILE *probe = fopen(vfs_path, "rb");
    if (probe == nullptr) {
        webui_log_appendf("error: file not found %s\n", vfs_path);
        return false;
    }
    fclose(probe);
    const bool ok = ui_cnc_print_service_execute_gcode_file(vfs_path);
    webui_log_appendf("run file %s\n", vfs_path);
    return ok;
}

void webui_cnc_get_position(float *x_mm, float *y_mm)
{
    ui_cnc_motion_facade_get_position_mm(x_mm, y_mm);
}

bool webui_cnc_is_busy(void)
{
    return ui_cnc_print_service_is_busy();
}

void webui_cnc_pause(void)
{
    ui_cnc_print_service_pause();
    webui_log_append("pause\n");
}

void webui_cnc_run(void)
{
    ui_cnc_print_service_run();
    webui_log_append("run\n");
}

void webui_cnc_apply_settings(void)
{
    ui_cnc_print_service_apply_settings();
}

bool webui_cnc_has_suspended_job(void)
{
    return ui_cnc_print_service_has_suspended_job();
}

#else

bool webui_cnc_available(void) { return false; }
bool webui_cnc_worker_ready(void) { return false; }
bool webui_cnc_submit_line(const char *line) { (void)line; return false; }
bool webui_cnc_jog(const char *axis_delta, float feed_mm_min)
{
    (void)axis_delta;
    (void)feed_mm_min;
    return false;
}
bool webui_cnc_home(void) { return false; }
bool webui_cnc_move_to_mm_async(float x_mm, float y_mm)
{
    (void)x_mm;
    (void)y_mm;
    return false;
}
bool webui_cnc_run_file(const char *vfs_path)
{
    (void)vfs_path;
    return false;
}
void webui_cnc_get_position(float *x_mm, float *y_mm) { if (x_mm) *x_mm = 0; if (y_mm) *y_mm = 0; }
bool webui_cnc_is_busy(void) { return false; }
void webui_cnc_pause(void) {}
void webui_cnc_run(void) {}
void webui_cnc_apply_settings(void) {}
bool webui_cnc_has_suspended_job(void) { return false; }

#endif
