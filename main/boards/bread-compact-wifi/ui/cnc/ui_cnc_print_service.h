#pragma once

#include "../laser_ui_events.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 注册 UI 事件：点动/运行/暂停/设置应用 → 本地 GPIO 步进 + 激光 PWM。 */
void ui_cnc_print_service_init(void);

/** 解析并执行 G-code（G0/G1/M3/M5/G1 F），不自动回原点。 */
void ui_cnc_print_service_execute_gcode(const char *gcode_text);

/** 快速移动到绝对坐标 / 回零（入队 ui_cnc_worker，勿阻塞 LVGL）。 */
void ui_cnc_print_service_move_to_mm_async(float x_mm, float y_mm);
void ui_cnc_print_service_home_async(void);

bool ui_cnc_print_service_is_busy(void);

/** 由 ui_evt 任务调用，勿在 LVGL 线程同步执行步进。 */
void ui_cnc_print_service_on_event(laser_ui_event_id_t id);

#ifdef __cplusplus
}
#endif
