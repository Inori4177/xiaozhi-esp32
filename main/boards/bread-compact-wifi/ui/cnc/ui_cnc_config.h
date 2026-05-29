#pragma once

/** UI-side CNC constants (mirror KanjiVG / boards/CNC; do not include gcode_controller.h). */

#define UI_CNC_WORK_SIZE_MM       42.0f

#define UI_CNC_STEPS_PER_MM_X     106.666f
#define UI_CNC_STEPS_PER_MM_Y     106.666f
#define UI_CNC_MAX_RATE_MM_MIN    700.0f
#define UI_CNC_ACCELERATION       800.0f
#define UI_CNC_JUNCTION_DEV       0.02f
#define UI_CNC_MAX_TRAVEL_MM      42.0f

#define UI_CNC_RAPID_FEED_MM_MIN  3000.0f

/** 设置页速度 100% 对应进给 (mm/min)，与 laser_ui_state / LASER_GCODE_FEED_BASE 一致 */
#define UI_CNC_FEED_BASE_MM_MIN   1000

/** FreeRTOS task stack for pick move (bytes). */
#define UI_CNC_MOVE_TASK_STACK    4096
#define UI_CNC_MOVE_TASK_PRIO     5

/** CNC worker：G-code 解析 + 步进阻塞不可在 taskLVGL 内执行。 */
#define UI_CNC_WORK_TASK_STACK    8192
#define UI_CNC_WORK_QUEUE_LEN     8
#define UI_CNC_WORK_GCODE_MAX     512
/** 运动/UI 后台任务固定 Core 0，Core 1 仅 taskLVGL */
#define UI_CNC_TASK_CORE          0
