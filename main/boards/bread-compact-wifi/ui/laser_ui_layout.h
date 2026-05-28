#pragma once

#include <lvgl.h>

// Screen layout (480 x 320 landscape)
#define UI_TOP_H        32
#define UI_BOTTOM_H     28
#define UI_MAIN_H       (LV_VER_RES - UI_TOP_H - UI_BOTTOM_H)

// Left floating dock (vertical column)
#define UI_NAV_DOCK_W       52
#define UI_NAV_DOCK_BTN     46
/* 达到阈值即自动展开/折叠完整页面，无需拖到底 */
#define UI_NAV_DRAG_OPEN_PX         18
#define UI_NAV_DRAG_CLOSE_PX        18
#define UI_NAV_DRAG_RELEASE_OPEN_PX  10
#define UI_NAV_DRAG_RELEASE_CLOSE_PX 10
#define UI_NAV_DRAG_FOLLOW_MAX      16
#define UI_CONTENT_W        (LV_HOR_RES - UI_NAV_DOCK_W)

/* 页面侧滑动画时长 (ms) */
#define UI_PAGE_OPEN_ANIM_MS   180
#define UI_PAGE_CLOSE_ANIM_MS   90

// Main UI palette — solid black background (laser_ui_apply_main_gradient)
#define UI_COLOR_BG_BLACK     lv_color_hex(0x000000)
#define UI_COLOR_BG_BLUE      lv_color_hex(0x2B52D0)
#define UI_COLOR_BG           UI_COLOR_BG_BLACK
#define UI_COLOR_PANEL        lv_color_hex(0x141A28)
#define UI_COLOR_NAV          lv_color_hex(0x181E30)
#define UI_COLOR_NAV_ACTIVE   lv_color_hex(0x244878)
#define UI_COLOR_BORDER       lv_color_hex(0x3A5080)
#define UI_COLOR_TEXT         lv_color_hex(0xE8EEF5)
#define UI_COLOR_TEXT_DIM     lv_color_hex(0x8FA3BC)
#define UI_COLOR_RUN          lv_color_hex(0x4CAF50)
#define UI_COLOR_PAUSE        lv_color_hex(0xFF9800)
#define UI_COLOR_IDLE         lv_color_hex(0x607D8B)
#define UI_COLOR_ALARM        lv_color_hex(0xF44336)
#define UI_COLOR_ACCENT       lv_color_hex(0x5C9DFF)

/* Print status card — higher contrast on dark panel */
#define UI_COLOR_STATUS_BG        lv_color_hex(0x121828)
#define UI_COLOR_STATUS_BORDER    lv_color_hex(0x3D5A80)

#define UI_COLOR_STATUS_TITLE     lv_color_hex(0xF0F4F8)
#define UI_COLOR_STATUS_META      lv_color_hex(0x9FB3C8)
#define UI_COLOR_STATUS_VALUE     lv_color_hex(0x64B5F6)
#define UI_COLOR_STATUS_IDLE_BG   lv_color_hex(0x37474F)
#define UI_COLOR_STATUS_IDLE_FG   lv_color_hex(0xECEFF1)

// Cyber splash palette
#define SPLASH_BG             lv_color_hex(0x000000)
#define CYBER_BG              lv_color_hex(0x0A0A12)
#define CYBER_GRID            lv_color_hex(0x1A1A2E)
#define CYBER_CYAN            lv_color_hex(0x00F5FF)
#define CYBER_MAGENTA         lv_color_hex(0xFF00AA)
#define CYBER_GREEN           lv_color_hex(0x39FF14)
#define CYBER_AMBER           lv_color_hex(0xFFB800)
#define CYBER_DIM             lv_color_hex(0x4A5568)
#define CYBER_RED             lv_color_hex(0xFF3366)

void laser_ui_apply_main_gradient(lv_obj_t *obj);
