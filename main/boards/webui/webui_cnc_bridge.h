#pragma once

#ifdef __cplusplus
extern "C" {
#endif

bool webui_cnc_available(void);
bool webui_cnc_worker_ready(void);
bool webui_cnc_submit_line(const char *line);
bool webui_cnc_jog(const char *axis_delta, float feed_mm_min);
bool webui_cnc_home(void);
bool webui_cnc_move_to_mm_async(float x_mm, float y_mm);
bool webui_cnc_run_file(const char *vfs_path);
void webui_cnc_get_position(float *x_mm, float *y_mm);
bool webui_cnc_is_busy(void);
void webui_cnc_pause(void);
void webui_cnc_run(void);
void webui_cnc_apply_settings(void);
bool webui_cnc_has_suspended_job(void);

#ifdef __cplusplus
}
#endif
