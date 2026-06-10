#pragma once

#include <lvgl.h>

// ============================================================================
// 屏幕布局 (480×320 横屏) — Screen layout constants
// ============================================================================
#define UI_TOP_H        28  // 顶部状态栏高度 (chrome area)
#define UI_BOTTOM_H     20   // 底部状态栏高度
#define UI_MAIN_H       (LV_VER_RES - UI_TOP_H - UI_BOTTOM_H)  // 主区域 = 276px

// 顶部 overlay 居中标题（lcd_display status_label_）
#define UI_CHROME_STATUS_TEXT   "激光雕刻机"

// 底部 overlay 文字区（lcd_display bottom_bar_ / chat_message_label_）
#define UI_BOTTOM_BAR_W         LV_HOR_RES
#define UI_BOTTOM_BAR_H         UI_BOTTOM_H
#define UI_BOTTOM_BAR_OFS_X     0   // 相对屏幕底边中点的水平偏移 (px)
#define UI_BOTTOM_BAR_OFS_Y     0   // 相对屏幕底边中点的垂直偏移 (px，负=更贴底)
#define UI_BOTTOM_TEXT_W        (LV_HOR_RES - 32)
#define UI_BOTTOM_TEXT_H        0  // 0 = 随字体行高；>0 固定标签高度
#define UI_BOTTOM_TEXT_OFS_X    0   // 相对底栏中心的水平偏移 (px)
#define UI_BOTTOM_TEXT_OFS_Y    0   // 相对底栏中心的垂直偏移 (px)

// ============================================================================
// 生物拟态导航浮标系统 — Biomimetic navigation buoy system
// ============================================================================

/* 浮标尺寸与位置 — 针对 480×320 横屏优化 */
#define UI_BUOY_R            11   // 浮标圆形半径 (22px直径)
#define UI_BUOY_HIT_R        22   // 触控热区半径（扩展不可见触控区域到 44px）
#define UI_BUOY_MARGIN_L     4    // 距左边缘
#define UI_BUOY_COLLAPSED_X   0    // 折叠态 X 中心：贴在左边缘，只露出右半圆
#define UI_BUOY_EXPANDED_X    (UI_BUOY_R + UI_BUOY_MARGIN_L)  // 展开态 X 中心（与折叠同侧）
#define UI_BUOY_GAP           50   // 展开后各浮标之间的间隙 (10dp touch spacing)
#define UI_BUOY_COUNT         3    // 三个页面：Print / Settings / Pick
#define UI_BUOY_DRAG_Y_RANGE  50   // 折叠态浮标可垂直拖拽范围 (px)

/* ---------- 呼吸动画（模拟水母游动节奏） ---------- */
#define UI_BUOY_BREATHE_AMP_MIN  0.5f   // 振幅最小 px
#define UI_BUOY_BREATHE_AMP_MAX  1.2f   // 振幅最大 px
#define UI_BUOY_BREATHE_FREQ_HZ  0.8f   // 呼吸频率 0.8Hz（周期约 1.25s）
#define UI_BUOY_BREATHE_PERIOD_MS  ((int)(1000.0f / UI_BUOY_BREATHE_FREQ_HZ))  // ≈1250ms

/* ---------- 按压反馈 ---------- */
#define UI_BUOY_PRESS_SCALE      90    // 按压缩小到 90%（与按钮的 92% 区分，体现浮标质感）
#define UI_BUOY_PRESS_DURATION   100   // 缩小动画时长 ms
#define UI_BUOY_RELEASE_DURATION 180   // 回弹动画时长 ms

/* ---------- 弹性拖拽形变 ---------- */
#define UI_BUOY_DRAG_DAMPING    0.7f    // 阻尼系数 (0.6–0.8)，值越小越有"粘滞"感
#define UI_BUOY_DRAG_MAX_FOLLOW  24     // 拖拽最大跟随距离 px
#define UI_BUOY_DRAG_EDGE_BOUNCE_MS  150  // 边缘碰撞回弹动画时长 ms

/* ---------- 拖拽切换阈值 ---------- */
#define UI_BUOY_DRAG_OPEN_PX          12   // 拖拽超过此值自动展开
#define UI_BUOY_DRAG_RELEASE_OPEN_PX   8   // 松手时最小展开距离
#define UI_BUOY_DRAG_CLOSE_PX         12   // 反向拖拽超过此值折叠
#define UI_BUOY_DRAG_RELEASE_CLOSE_PX  8   // 松手时最小折叠距离

/* ---------- 展开/折叠动画 ---------- */
#define UI_BUOY_EXPAND_MS       240   // 展开动画时长 (ms)
#define UI_BUOY_COLLAPSE_MS     180   // 折叠动画时长 (ms)
#define UI_BUOY_STAGGER_MS      50    // 每个浮标展开的错开间隔 (ms)

/* 内容区域（浮标展开后占左方空间，内容区自适应） */
#define UI_NAV_EXPANDED_W       (UI_BUOY_R * 2 + UI_BUOY_MARGIN_L * 2 + 6)
#define UI_NAV_COLLAPSED_W      (UI_BUOY_R * 2 + UI_BUOY_MARGIN_L * 2)
#define UI_CONTENT_W            (LV_HOR_RES - UI_NAV_EXPANDED_W)  // 内容区宽度

/* ---------- 页面切换动画 ---------- */
#undef UI_CONTENT_W
#define UI_CONTENT_X            54
#define UI_CONTENT_W            426
#define UI_PAGE_OPEN_ANIM_MS   200
#define UI_PAGE_CLOSE_ANIM_MS  100

/* 语音 AI 页 — 小智 walk 精灵 (77×99，原地帧 + 代码平移) */
#define UI_XIAOZHI_SPRITE_W       77
#define UI_XIAOZHI_SPRITE_H       99
#define UI_XIAOZHI_WALK_MARGIN_X  16
#define UI_XIAOZHI_WALK_MIN_X     UI_XIAOZHI_WALK_MARGIN_X
#define UI_XIAOZHI_WALK_MAX_X     (UI_CONTENT_W - UI_XIAOZHI_SPRITE_W - UI_XIAOZHI_WALK_MARGIN_X)
#define UI_XIAOZHI_WALK_Y         ((UI_MAIN_H - UI_XIAOZHI_SPRITE_H) / 2)
#define UI_XIAOZHI_WALK_FRAME_MS  80
#define UI_XIAOZHI_WALK_STEP_X    2

/* 语音 AI 页 — RUN 挥手精灵 (82×99，内容区居中) */
#define UI_XIAOZHI_RUN_SPRITE_W   82
#define UI_XIAOZHI_RUN_SPRITE_H   99
#define UI_XIAOZHI_RUN_CENTER_X   ((UI_CONTENT_W - UI_XIAOZHI_RUN_SPRITE_W) / 2)
#define UI_XIAOZHI_RUN_CENTER_Y   ((UI_MAIN_H - UI_XIAOZHI_RUN_SPRITE_H) / 2)
#define UI_XIAOZHI_RUN_FRAME_MS   80

/* ---------- 宸︿晶鍥剧墖瀵艰埅 ---------- */
#define UI_NAV_IMAGE_X          0
#define UI_NAV_IMAGE_Y          8
#define UI_NAV_IMAGE_W          53
#define UI_NAV_IMAGE_H          260
#define UI_NAV_HOTSPOT_X        4
#define UI_NAV_HOTSPOT_W        45
#define UI_NAV_HOTSPOT_H        44
#define UI_NAV_HOTSPOT_Y_RUN    29
#define UI_NAV_HOTSPOT_Y_SETTINGS 87
#define UI_NAV_HOTSPOT_Y_PICK   145
#define UI_NAV_HOTSPOT_Y_VOICE  202
#define UI_NAV_HOTSPOT_EXTEND   6

/* ---------- 选定页 / 设置页 PNG 按键 ---------- */
#define UI_PICK_BTN_W           90
#define UI_PICK_BTN_H           45
#define UI_PICK_BTN_EXT_CLICK   6
#define UI_PICK_BTN_GAP         12
#define UI_PICK_RAIL_GAP        8
#define UI_PICK_STATUS_H        24
#define UI_PICK_STATUS_TO_BTN   8
#define UI_SETTINGS_APPLY_W     160
#define UI_SETTINGS_APPLY_H     52
#define UI_SETTINGS_APPLY_X     133
#define UI_SETTINGS_APPLY_Y     196
#define UI_SETTINGS_APPLY_EXT_CLICK 8

// ============================================================================
// 主 UI 调色板 (暗色主题 — Dark theme with cyan accent)
// ============================================================================

/* 基础色 */
#define UI_COLOR_PAGE_BG_TOP      lv_color_hex(0xD9F3EA)
#define UI_COLOR_PAGE_BG_BOTTOM   lv_color_hex(0xFFD8C8)
#define UI_COLOR_CARD             lv_color_hex(0xFFFDF9)
#define UI_COLOR_CARD_ALT         lv_color_hex(0xF4FCF9)
#define UI_COLOR_CARD_PEACH       lv_color_hex(0xFFF0E8)
#define UI_COLOR_SOFT_MINT        lv_color_hex(0xBFEADF)
#define UI_COLOR_MINT             lv_color_hex(0x42CDBE)
#define UI_COLOR_AQUA             lv_color_hex(0x1EAFA2)
#define UI_COLOR_PEACH            lv_color_hex(0xFFD4C3)
#define UI_COLOR_CORAL            lv_color_hex(0xFF7B76)
#define UI_COLOR_INK              lv_color_hex(0x102846)
#define UI_COLOR_MUTED            lv_color_hex(0x314A64)
#define UI_COLOR_FAINT            lv_color_hex(0x586B7F)
#define UI_COLOR_LIGHT_BORDER     lv_color_hex(0xD9F1EB)
#define UI_COLOR_LIGHT_LINE       lv_color_hex(0xE6EEF2)

#define UI_COLOR_BG_BLACK     UI_COLOR_PAGE_BG_BOTTOM
#define UI_COLOR_BG_DEEP      UI_COLOR_PAGE_BG_TOP
#define UI_COLOR_SURFACE      UI_COLOR_CARD
#define UI_COLOR_SURFACE_RAISED UI_COLOR_CARD_ALT
#define UI_COLOR_NAV_BG       lv_color_hex(0x111420)   // 导航区背景
/* 向后兼容: 原 UI_COLOR_PANEL (splash 进度条背景等仍使用) */
#define UI_COLOR_PANEL        UI_COLOR_SURFACE

/* 浮标配色 — 青绿色系 */
#define UI_COLOR_BUOY_IDLE    lv_color_hex(0x1A8A80)   // 浮标默认色
#define UI_COLOR_BUOY_ACTIVE  lv_color_hex(0x2EC4B6)   // 浮标激活色
#define UI_COLOR_BUOY_PRESS   lv_color_hex(0x3DD6C8)   // 浮标按压色
#define UI_COLOR_BUOY_GLOW    lv_color_hex(0x70E8DC)   // 浮标发光色

/* 边框与分割线 */
#define UI_COLOR_BORDER       UI_COLOR_LIGHT_LINE
#define UI_COLOR_BORDER_GLOW  UI_COLOR_LIGHT_BORDER

/* 文字色 */
#define UI_COLOR_TEXT         UI_COLOR_INK
#define UI_COLOR_TEXT_SEC     UI_COLOR_MUTED
#define UI_COLOR_TEXT_DIM     UI_COLOR_FAINT

/* 强调色 (青/蓝) */
#define UI_COLOR_ACCENT       UI_COLOR_AQUA
#define UI_COLOR_ACCENT_DIM   UI_COLOR_SOFT_MINT
#define UI_COLOR_ACCENT_GLOW  lv_color_hex(0xA8EDE4)

/* 功能色 */
#define UI_COLOR_RUN          lv_color_hex(0x32C79B)
#define UI_COLOR_RUN_DIM      lv_color_hex(0xDFF7EF)
#define UI_COLOR_PAUSE        lv_color_hex(0xFFAD70)
#define UI_COLOR_PAUSE_DIM    lv_color_hex(0xFFE8D9)
#define UI_COLOR_IDLE         lv_color_hex(0x86A2B6)
#define UI_COLOR_ALARM        UI_COLOR_CORAL

/* 状态面板专用 */
#define UI_COLOR_STATUS_BG        UI_COLOR_CARD
#define UI_COLOR_STATUS_BORDER    UI_COLOR_LIGHT_BORDER
#define UI_COLOR_STATUS_TITLE     UI_COLOR_INK
#define UI_COLOR_STATUS_VALUE     UI_COLOR_AQUA
#define UI_COLOR_STATUS_META      UI_COLOR_MUTED
/* 空闲状态徽章 (ui_cnc_print_status_service.cc 引用) */
#define UI_COLOR_STATUS_IDLE_BG   lv_color_hex(0xE8F7F1)
#define UI_COLOR_STATUS_IDLE_FG   lv_color_hex(0x2AAE91)

// ============================================================================

// ============================================================================
// 公共样式函数声明
// ============================================================================
// Splash screen compatibility palette.
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
