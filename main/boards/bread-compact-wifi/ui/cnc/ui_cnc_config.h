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

/** FreeRTOS task stack for pick move (bytes). */
#define UI_CNC_MOVE_TASK_STACK    4096
#define UI_CNC_MOVE_TASK_PRIO     5
