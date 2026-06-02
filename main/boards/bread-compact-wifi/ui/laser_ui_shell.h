#pragma once

#include <lvgl.h>

/**
 * 页面枚举 — 与原有架构完全兼容
 */
enum class LaserPage {
    Print = 0,
    Settings,
    Pick,
    Count,
};

/**
 * 激光 UI 主外壳 — 管理导航浮标系统和页面切换
 *
 * 导航浮标设计理念：
 *   折叠态 → 单个呼吸态浮标悬浮在左侧
 *   展开态 → 三个浮标垂直排列，点击切页后自动折叠回单个
 *
 * 交互特性：
 *   - 呼吸动画 (0.5-1.2px 振幅, 0.8Hz)
 *   - 按压形变反馈
 *   - 拖拽弹性跟随 (阻尼 0.6-0.8)
 *   - 边缘碰撞回弹
 */
struct LaserUiShell {
    lv_obj_t *root = nullptr;           // 根容器
    lv_obj_t *content_host = nullptr;   // 页面内容宿主
    lv_obj_t *buoy_layer = nullptr;     // 浮标层（透明覆盖层）
    lv_obj_t *buoys[3] = {};            // 三个导航浮标
    lv_obj_t *pages[3] = {};            // 三个页面（Print / Settings / Pick）
    LaserPage current = LaserPage::Print;
    bool page_open = false;

    /* 浮标展开/折叠状态 */
    bool buoys_expanded = false;        // 浮标是否处于展开态
    bool buoys_animating = false;       // 是否正在执行展开/折叠动画

    /* 拖拽状态追踪 */
    int nav_drag_origin_x = 0;          // 拖拽起始 X 坐标
    int nav_drag_page = -1;             // 当前被拖拽的页面索引
    bool nav_drag_snapped = false;      // 是否已触发 snap 阈值
    bool nav_suppress_click = false;    // 拖拽后抑制 click 事件
    int nav_drag_follow = 0;            // 当前拖拽跟随距离
};

/**
 * 初始化 UI 外壳
 * @param shell  外壳对象指针
 * @param screen LVGL 活动屏幕
 */
void laser_ui_shell_init(LaserUiShell *shell, lv_obj_t *screen);

/**
 * 无动画切换页面 (用于外部调用)
 */
void laser_ui_shell_switch_page(LaserUiShell *shell, LaserPage page);
