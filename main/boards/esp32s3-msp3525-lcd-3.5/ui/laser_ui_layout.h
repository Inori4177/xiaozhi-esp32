#pragma once

#include <lvgl.h>

// Screen layout (480 x 320 landscape)
#define UI_TOP_H        32
#define UI_BOTTOM_H     28
#define UI_MAIN_H       (LV_VER_RES - UI_TOP_H - UI_BOTTOM_H)
#define UI_NAV_W        72
#define UI_CONTENT_W    (LV_HOR_RES - UI_NAV_W)

// Main UI dark palette
#define UI_COLOR_BG           lv_color_hex(0x121212)
#define UI_COLOR_PANEL        lv_color_hex(0x1E1E1E)
#define UI_COLOR_NAV          lv_color_hex(0x181818)
#define UI_COLOR_NAV_ACTIVE   lv_color_hex(0x2D5A3D)
#define UI_COLOR_BORDER       lv_color_hex(0x333333)
#define UI_COLOR_TEXT         lv_color_hex(0xE0E0E0)
#define UI_COLOR_TEXT_DIM     lv_color_hex(0x888888)
#define UI_COLOR_RUN          lv_color_hex(0x4CAF50)
#define UI_COLOR_PAUSE        lv_color_hex(0xFF9800)
#define UI_COLOR_IDLE         lv_color_hex(0x757575)
#define UI_COLOR_ALARM        lv_color_hex(0xF44336)
#define UI_COLOR_ACCENT       lv_color_hex(0x2196F3)

/* Print status card — higher contrast on dark panel */
#define UI_COLOR_STATUS_BG        lv_color_hex(0x161B22)
#define UI_COLOR_STATUS_BORDER    lv_color_hex(0x3D4F5F)
#define UI_COLOR_STATUS_TITLE     lv_color_hex(0xF0F4F8)
#define UI_COLOR_STATUS_META      lv_color_hex(0x9FB3C8)
#define UI_COLOR_STATUS_VALUE     lv_color_hex(0x4FC3F7)
#define UI_COLOR_STATUS_IDLE_BG   lv_color_hex(0x455A64)
#define UI_COLOR_STATUS_IDLE_FG   lv_color_hex(0xECEFF1)

// Cyber splash palette
#define CYBER_BG              lv_color_hex(0x0A0A12)
#define CYBER_GRID            lv_color_hex(0x1A1A2E)
#define CYBER_CYAN            lv_color_hex(0x00F5FF)
#define CYBER_MAGENTA         lv_color_hex(0xFF00AA)
#define CYBER_GREEN           lv_color_hex(0x39FF14)
#define CYBER_AMBER           lv_color_hex(0xFFB800)
#define CYBER_DIM             lv_color_hex(0x4A5568)
#define CYBER_RED             lv_color_hex(0xFF3366)
