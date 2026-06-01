#pragma once

#include "../laser_ui_events.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UI_CNC_WORK_IDLE = 0,
    UI_CNC_WORK_RUNNING,
    UI_CNC_WORK_JOGGING,
    UI_CNC_WORK_PAUSED,
} ui_cnc_work_state_t;

typedef struct {
    ui_cnc_work_state_t state;
    uint8_t progress_pct;
    uint32_t elapsed_sec;
    uint32_t eta_sec;
    bool has_eta;
    bool is_print_job;
} ui_cnc_print_status_t;

/** 注册 UI 事件：点动/运行/暂停/设置应用 → 本地 GPIO 步进 + 激光 PWM。 */
void ui_cnc_print_service_init(void);

/** 解析并执行 G-code（G0/G1/M3/M5/G1 F），不自动回原点。 */
bool ui_cnc_print_service_execute_gcode(const char *gcode_text);

/** 快速移动到绝对坐标 / 回零（入队 ui_cnc_worker，勿阻塞 LVGL）。 */
bool ui_cnc_print_service_move_to_mm_async(float x_mm, float y_mm);
bool ui_cnc_print_service_home_async(void);

/** WebUI / 外部点动：按绝对步长移动单轴（不拼 G-code 字符串）。 */
bool ui_cnc_print_service_jog_axis_mm(char axis, bool positive, float step_mm);

/** 从 VFS 路径读取 G-code 文件并在 worker 中执行。 */
bool ui_cnc_print_service_execute_gcode_file(const char *vfs_path);

/** CNC worker 任务是否已创建（入队命令前可检查）。 */
bool ui_cnc_print_service_worker_ready(void);

bool ui_cnc_print_service_is_busy(void);

/** 当前/最近一次作业显示名（文件 basename 或 "G-code"）。 */
const char *ui_cnc_print_service_get_job_name(void);

void ui_cnc_print_service_get_status(ui_cnc_print_status_t *out);

void ui_cnc_print_service_get_position_mm(float *x_mm, float *y_mm);

/** MotionController / 雕刻任务：作业开始前预扫描 G-code 估算进度。 */
void ui_cnc_print_service_notify_job_begin(const char *gcode_text);

/** 命令执行进度（done 为已完成条数，total 为总条数）。 */
void ui_cnc_print_service_notify_job_progress(size_t done, size_t total);

void ui_cnc_print_service_notify_job_end(void);

/** 由 ui_evt 任务调用，勿在 LVGL 线程同步执行步进。 */
void ui_cnc_print_service_on_event(laser_ui_event_id_t id);

/** 暂停：关激光、中止步进，保留未执行的 G-code 以便继续。 */
void ui_cnc_print_service_pause(void);

/** 运行/继续：无挂起作业时按 UI 设置开激光；有挂起作业则继续剩余 G-code。 */
void ui_cnc_print_service_run(void);

/** 将 laser_ui_state 功率/速度下发到运动层 (M3 S + G1 F)。 */
void ui_cnc_print_service_apply_settings(void);

/** 是否有已暂停、可继续的 G-code 作业。 */
bool ui_cnc_print_service_has_suspended_job(void);

/** 步进层轮询：暂停请求已发出时中止当前运动段。 */
bool ui_cnc_print_service_poll_pause_abort(void);

#ifdef __cplusplus
}
#endif
