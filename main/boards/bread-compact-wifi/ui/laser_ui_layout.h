#pragma once

#include <lvgl.h>

// ============================================================================
// 屏幕布局 (480×320 横屏) — Screen layout constants
// ============================================================================
#define UI_TOP_H        24   // 顶部状态栏高度 (chrome area)
#define UI_BOTTOM_H     20   // 底部状态栏高度
#define UI_MAIN_H       (LV_VER_RES - UI_TOP_H - UI_BOTTOM_H)  // 主区域 = 276px

// ============================================================================
// 生物拟态导航浮标系统 — Biomimetic navigation buoy system
// ============================================================================

/* 浮标尺寸与位置 — 针对 480×320 横屏优化 */
#define UI_BUOY_R            13   // 浮标圆形半径 (26px直径, 可见性好)
#define UI_BUOY_HIT_R        22   // 触控热区半径（扩展不可见触控区域到 44px）
#define UI_BUOY_MARGIN_L     4    // 距左边缘
#define UI_BUOY_COLLAPSED_X   (UI_BUOY_R + UI_BUOY_MARGIN_L)  // 折叠态 X 中心
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
#define UI_PAGE_OPEN_ANIM_MS   200
#define UI_PAGE_CLOSE_ANIM_MS  100

// ============================================================================
// 主 UI 调色板 (暗色主题 — Dark theme with cyan accent)
// ============================================================================

/* 基础色 */
#define UI_COLOR_BG_BLACK     lv_color_hex(0x06080D)   // 最深底色
#define UI_COLOR_BG_DEEP      lv_color_hex(0x0C0F18)   // 页面深层背景
#define UI_COLOR_SURFACE      lv_color_hex(0x141825)   // 卡片/面板表面
#define UI_COLOR_SURFACE_RAISED lv_color_hex(0x1A1F30) // 悬浮面板
#define UI_COLOR_NAV_BG       lv_color_hex(0x111420)   // 导航区背景
/* 向后兼容: 原 UI_COLOR_PANEL (splash 进度条背景等仍使用) */
#define UI_COLOR_PANEL        UI_COLOR_SURFACE

/* 浮标配色 */
#define UI_COLOR_BUOY_IDLE    lv_color_hex(0x1C2545)   // 浮标默认色
#define UI_COLOR_BUOY_ACTIVE  lv_color_hex(0x2D4A8A)   // 浮标激活色
#define UI_COLOR_BUOY_PRESS   lv_color_hex(0x3A5090)   // 浮标按压色
#define UI_COLOR_BUOY_GLOW    lv_color_hex(0x3D7BFF)   // 浮标发光色

/* 边框与分割线 */
#define UI_COLOR_BORDER       lv_color_hex(0x222840)   // 普通边框
#define UI_COLOR_BORDER_GLOW  lv_color_hex(0x3D5A90)   // 发光边框

/* 文字色 */
#define UI_COLOR_TEXT         lv_color_hex(0xE8EDF5)   // 主文字
#define UI_COLOR_TEXT_SEC     lv_color_hex(0x9AA4BF)   // 次要文字
#define UI_COLOR_TEXT_DIM     lv_color_hex(0x6B7590)   // 暗淡文字

/* 强调色 (青/蓝) */
#define UI_COLOR_ACCENT       lv_color_hex(0x4D9FFF)   // 主强调色
#define UI_COLOR_ACCENT_DIM   lv_color_hex(0x2A60B0)   // 暗强调色
#define UI_COLOR_ACCENT_GLOW  lv_color_hex(0x6DB5FF)   // 发光强调色

/* 功能色 */
#define UI_COLOR_RUN          lv_color_hex(0x22C55E)   // 运行 / 绿色
#define UI_COLOR_RUN_DIM      lv_color_hex(0x166534)   // 暗绿
#define UI_COLOR_PAUSE        lv_color_hex(0xF59E0B)   // 暂停 / 琥珀
#define UI_COLOR_PAUSE_DIM    lv_color_hex(0x92400E)   // 暗琥珀
#define UI_COLOR_IDLE         lv_color_hex(0x64748B)   // 空闲 / 灰蓝
#define UI_COLOR_ALARM        lv_color_hex(0xEF4444)   // 警告 / 红

/* 状态面板专用 */
#define UI_COLOR_STATUS_BG        lv_color_hex(0x101522)
#define UI_COLOR_STATUS_BORDER    lv_color_hex(0x263450)
#define UI_COLOR_STATUS_TITLE     lv_color_hex(0xF0F4F8)
#define UI_COLOR_STATUS_VALUE     lv_color_hex(0x60A5FA)
#define UI_COLOR_STATUS_META      lv_color_hex(0x94A3B8)
/* 空闲状态徽章 (ui_cnc_print_status_service.cc 引用) */
#define UI_COLOR_STATUS_IDLE_BG   lv_color_hex(0x37474F)
#define UI_COLOR_STATUS_IDLE_FG   lv_color_hex(0xECEFF1)

// ============================================================================
// 赛博朋克 splash 屏调色板 (保持不变)
// ============================================================================
#define SPLASH_BG             lv_color_hex(0x000000)
#define CYBER_BG              lv_color_hex(0x0A0A12)
#define CYBER_GRID            lv_color_hex(0x1A1A2E)
#define CYBER_CYAN            lv_color_hex(0x00F5FF)
#define CYBER_MAGENTA         lv_color_hex(0xFF00AA)
#define CYBER_GREEN           lv_color_hex(0x39FF14)
#define CYBER_AMBER           lv_color_hex(0xFFB800)
#define CYBER_DIM             lv_color_hex(0x4A5568)
#define CYBER_RED             lv_color_hex(0xFF3366)

// ============================================================================
// 公共样式函数声明
// ============================================================================
void laser_ui_apply_main_gradient(lv_obj_t *obj);
