#include "laser_ui_shell.h"
#include "laser_ui_layout.h"
#include "laser_ui_widgets.h"
#include "pages/page_print.h"
#include "pages/page_settings.h"
#include "pages/page_pick.h"

#include <font_awesome.h>
#include <stdlib.h>

// ============================================================================
//  辅助函数 — Utilities
// ============================================================================

/** 获取触控指针在 root 容器内的局部坐标 */
static void pointer_local_point(const LaserUiShell *shell, int *out_x, int *out_y)
{
    lv_indev_t *indev = lv_indev_active();
    if (indev == nullptr || shell->root == nullptr) {
        if (out_x) { *out_x = 0; }
        if (out_y) { *out_y = 0; }
        return;
    }
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    lv_area_t area;
    lv_obj_get_coords(shell->root, &area);
    if (out_x) { *out_x = p.x - area.x1; }
    if (out_y) { *out_y = p.y - area.y1; }
}

static int pointer_x_in_root(const LaserUiShell *shell)
{
    int x = 0;
    pointer_local_point(shell, &x, nullptr);
    return x;
}

static int clamp_int(int v, int lo, int hi)
{
    if (v < lo) { return lo; }
    if (v > hi) { return hi; }
    return v;
}

/** 将 buoy_layer 提升到最顶层 */
static void shell_raise_buoy_layer(LaserUiShell *shell)
{
    if (shell->buoy_layer != nullptr) {
        lv_obj_move_foreground(shell->buoy_layer);
    }
}

// ============================================================================
//  页面动画 — Page slide animations
// ============================================================================

static void page_anim_exec(void *obj, int32_t v)
{
    lv_obj_set_x(static_cast<lv_obj_t *>(obj), v);
}

static void page_close_anim_ready(lv_anim_t *anim)
{
    if (anim != nullptr && anim->var != nullptr) {
        lv_obj_add_flag(static_cast<lv_obj_t *>(anim->var), LV_OBJ_FLAG_HIDDEN);
    }
}

static void shell_set_page_offset(LaserUiShell *shell, LaserPage page, int x_ofs, bool show)
{
    lv_obj_t *page_obj = shell->pages[static_cast<int>(page)];
    if (page_obj == nullptr) { return; }
    if (show) {
        lv_obj_remove_flag(page_obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(page_obj, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_set_x(page_obj, x_ofs);
}

static void shell_collapse_buoys(LaserUiShell *shell);

/** 打开指定页面 — 核心页面切换逻辑
 *
 *  注意: 此函数不再自动折叠浮标。调用者 (CLICKED handler) 应在调用
 *  此函数之前或之后显式管理浮标的展开/折叠状态，以避免竞态条件。
 */
static void shell_open_page(LaserUiShell *shell, LaserPage page, bool animate)
{
    if (shell == nullptr) { return; }

    if (shell->page_open && shell->current == page) {
        shell_raise_buoy_layer(shell);
        return;
    }

    /* 切换前通知旧页面 */
    if (shell->page_open && shell->current == LaserPage::Pick && page != LaserPage::Pick) {
        page_pick_on_hide();
    }
    if (shell->page_open && shell->current == LaserPage::Print && page != LaserPage::Print) {
        page_print_on_hide();
    }

    const int panel_w = lv_obj_get_width(shell->content_host);

    /* 隐藏所有非目标页面 */
    for (int i = 0; i < static_cast<int>(LaserPage::Count); ++i) {
        if (shell->pages[i] == nullptr || i == static_cast<int>(page)) { continue; }
        lv_obj_add_flag(shell->pages[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_x(shell->pages[i], -panel_w);
    }

    shell->current = page;
    shell->page_open = true;

    lv_obj_t *target = shell->pages[static_cast<int>(page)];
    if (target == nullptr) { return; }

    lv_obj_remove_flag(target, LV_OBJ_FLAG_HIDDEN);

    /* 更新浮标激活态 */
    for (int i = 0; i < static_cast<int>(LaserPage::Count); ++i) {
        if (shell->buoys[i] != nullptr) {
            laser_ui_buoy_set_active(shell->buoys[i], i == static_cast<int>(page));
        }
    }

    /* 新页面 on_show */
    if (page == LaserPage::Pick) { page_pick_on_show(); }
    if (page == LaserPage::Print) { page_print_on_show(); }

    if (!animate) {
        lv_obj_set_x(target, 0);
        shell_raise_buoy_layer(shell);
        return;
    }

    /* 侧滑入场 */
    lv_anim_del(target, page_anim_exec);
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, target);
    lv_anim_set_exec_cb(&anim, page_anim_exec);
    lv_anim_set_values(&anim, -panel_w, 0);
    lv_anim_set_duration(&anim, UI_PAGE_OPEN_ANIM_MS);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
    lv_anim_start(&anim);
    shell_raise_buoy_layer(shell);
}

/** 关闭当前页面 */
static void shell_close_page(LaserUiShell *shell, bool animate)
{
    if (shell == nullptr || !shell->page_open) { return; }

    if (shell->current == LaserPage::Pick) { page_pick_on_hide(); }
    if (shell->current == LaserPage::Print) { page_print_on_hide(); }

    const int panel_w = lv_obj_get_width(shell->content_host);
    lv_obj_t *target = shell->pages[static_cast<int>(shell->current)];
    if (target == nullptr) {
        shell->page_open = false;
        for (int i = 0; i < static_cast<int>(LaserPage::Count); ++i) {
            if (shell->buoys[i] != nullptr) {
                laser_ui_buoy_set_active(shell->buoys[i], false);
            }
        }
        return;
    }

    if (!animate) {
        for (int i = 0; i < static_cast<int>(LaserPage::Count); ++i) {
            shell_set_page_offset(shell, static_cast<LaserPage>(i), -panel_w, false);
        }
        shell->page_open = false;
        for (int i = 0; i < static_cast<int>(LaserPage::Count); ++i) {
            if (shell->buoys[i] != nullptr) {
                laser_ui_buoy_set_active(shell->buoys[i], false);
            }
        }
        shell_raise_buoy_layer(shell);
        return;
    }

    lv_anim_del(target, page_anim_exec);
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, target);
    lv_anim_set_exec_cb(&anim, page_anim_exec);
    lv_anim_set_values(&anim, lv_obj_get_x(target), -panel_w);
    lv_anim_set_duration(&anim, UI_PAGE_CLOSE_ANIM_MS);
    lv_anim_set_path_cb(&anim, lv_anim_path_linear);
    lv_anim_set_completed_cb(&anim, page_close_anim_ready);
    lv_anim_start(&anim);

    shell->page_open = false;
    for (int i = 0; i < static_cast<int>(LaserPage::Count); ++i) {
        if (shell->buoys[i] != nullptr) {
            laser_ui_buoy_set_active(shell->buoys[i], false);
        }
    }
    shell_raise_buoy_layer(shell);
}

// ============================================================================
//  浮标动画辅助回调 — Buoy animation helper callbacks (C function pointers)
// ============================================================================

/** marker 动画 no-op exec */
static void marker_noop_exec(void *var, int32_t v)
{
    (void)var; (void)v;
}

/** 展开动画完成 → 重置 buoys_animating 标志 */
static void expand_marker_done(lv_anim_t *a)
{
    if (a == nullptr || a->var == nullptr) { return; }
    lv_obj_t *layer = static_cast<lv_obj_t *>(a->var);
    auto *sh = static_cast<LaserUiShell *>(lv_obj_get_user_data(layer));
    if (sh != nullptr) { sh->buoys_animating = false; }
}

/** 折叠动画完成 → 隐藏非当前浮标，恢复呼吸
 *
 *  注意: 此回调在 shell_collapse_buoys 启动的标记动画完成时触发。
 *  此时 sh->current 已被 CLICKED handler 更新为正确的目标页面。
 *  每个浮标的独立 collapse 动画已完成 (buoy_collapse_ready_cb 已隐藏它们)，
 *  所以只需要重新显示当前页面的浮标并设置其位置和呼吸动画。
 */
static void collapse_marker_done(lv_anim_t *a)
{
    if (a == nullptr || a->var == nullptr) { return; }
    lv_obj_t *layer = static_cast<lv_obj_t *>(a->var);
    auto *sh = static_cast<LaserUiShell *>(lv_obj_get_user_data(layer));
    if (sh == nullptr) { return; }
    sh->buoys_animating = false;

    int cur_idx = static_cast<int>(sh->current);
    for (int i = 0; i < UI_BUOY_COUNT; ++i) {
        if (sh->buoys[i] == nullptr) { continue; }
        if (i == cur_idx) {
            /* 当前页浮标: 显示在折叠位，启动呼吸 */
            lv_obj_remove_flag(sh->buoys[i], LV_OBJ_FLAG_HIDDEN);
            /* 停止所有挂起的动画，避免旧动画干扰位置 */
            lv_anim_del(sh->buoys[i], nullptr);
            lv_obj_set_pos(sh->buoys[i],
                           UI_BUOY_COLLAPSED_X - UI_BUOY_R,
                           lv_obj_get_y(sh->buoys[i]));
            lv_obj_set_style_transform_scale(sh->buoys[i], 1000, LV_PART_MAIN);
            laser_ui_buoy_set_breathing(sh->buoys[i], true);
            laser_ui_buoy_set_active(sh->buoys[i], true);
        } else {
            /* 非当前页浮标: 确保隐藏 */
            lv_obj_add_flag(sh->buoys[i], LV_OBJ_FLAG_HIDDEN);
            laser_ui_buoy_set_breathing(sh->buoys[i], false);
        }
    }
    shell_raise_buoy_layer(sh);
}

/** 浮标 translate_x 动画 exec */
static void buoy_translate_exec(void *var, int32_t v)
{
    lv_obj_set_style_translate_x(static_cast<lv_obj_t *>(var), v, LV_PART_MAIN);
}

// ============================================================================
//  生物拟态浮标控制 — Biomimetic Buoy Control
// ============================================================================

/** 计算展开态各浮标的 Y 中心坐标 */
static void calc_buoy_expanded_positions(int *y_centers)
{
    const int main_h = UI_MAIN_H;
    const int buoy_diam = UI_BUOY_R * 2;
    const int total_h = buoy_diam * UI_BUOY_COUNT + UI_BUOY_GAP * (UI_BUOY_COUNT - 1);
    const int start_y = (main_h - total_h) / 2;

    for (int i = 0; i < UI_BUOY_COUNT; ++i) {
        y_centers[i] = start_y + UI_BUOY_R + i * (buoy_diam + UI_BUOY_GAP);
    }
}

/** 展开所有浮标 — staggered overshoot 动画 */
static void shell_expand_buoys(LaserUiShell *shell)
{
    if (shell == nullptr || shell->buoys_expanded || shell->buoys_animating) { return; }
    shell->buoys_expanded = true;
    shell->buoys_animating = true;

    /* 暂停当前浮标的呼吸动画 */
    if (shell->buoys[static_cast<int>(shell->current)] != nullptr) {
        laser_ui_buoy_set_breathing(shell->buoys[static_cast<int>(shell->current)], false);
    }

    int y_centers[UI_BUOY_COUNT];
    calc_buoy_expanded_positions(y_centers);

    for (int i = 0; i < UI_BUOY_COUNT; ++i) {
        if (shell->buoys[i] == nullptr) { continue; }
        int delay = i * UI_BUOY_STAGGER_MS;
        laser_ui_buoy_set_expanded(shell->buoys[i], true, delay,
                                   UI_BUOY_EXPANDED_X, y_centers[i]);
    }

    /* 动画结束后重置标志 */
    int total_dur = UI_BUOY_EXPAND_MS + (UI_BUOY_COUNT - 1) * UI_BUOY_STAGGER_MS + 50;
    lv_anim_t marker;
    lv_anim_init(&marker);
    lv_anim_set_var(&marker, shell->buoy_layer);
    lv_anim_set_duration(&marker, total_dur);
    lv_anim_set_values(&marker, 0, 1);
    lv_anim_set_exec_cb(&marker, marker_noop_exec);
    lv_anim_set_completed_cb(&marker, expand_marker_done);
    lv_anim_start(&marker);
}

/** 折叠所有浮标 — 回收为一个 */
static void shell_collapse_buoys(LaserUiShell *shell)
{
    if (shell == nullptr || !shell->buoys_expanded || shell->buoys_animating) { return; }
    shell->buoys_expanded = false;
    shell->buoys_animating = true;

    /* 计算折叠态 Y 中心（所有浮标回收到的同一位置） */
    int y_centers[UI_BUOY_COUNT];
    calc_buoy_expanded_positions(y_centers);
    const int collapsed_y = y_centers[1];  // 中间浮标的 Y 中心

    for (int i = 0; i < UI_BUOY_COUNT; ++i) {
        if (shell->buoys[i] == nullptr) { continue; }
        int delay = (UI_BUOY_COUNT - 1 - i) * UI_BUOY_STAGGER_MS;
        laser_ui_buoy_set_expanded(shell->buoys[i], false, delay,
                                   UI_BUOY_COLLAPSED_X, collapsed_y);
    }

    int total_dur = UI_BUOY_COLLAPSE_MS + (UI_BUOY_COUNT - 1) * UI_BUOY_STAGGER_MS + 50;
    lv_anim_t marker;
    lv_anim_init(&marker);
    lv_anim_set_var(&marker, shell->buoy_layer);
    lv_anim_set_duration(&marker, total_dur);
    lv_anim_set_values(&marker, 0, 1);
    lv_anim_set_exec_cb(&marker, marker_noop_exec);
    lv_anim_set_completed_cb(&marker, collapse_marker_done);
    lv_anim_start(&marker);
}

// ============================================================================
//  浮标交互事件 — Buoy event handler
// ============================================================================

/** 浮标弹性回弹 — overshoot 路径模拟弹簧效果 */
static void buoy_spring_back(lv_obj_t *buoy, int from_translate)
{
    lv_anim_del(buoy, buoy_translate_exec);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, buoy);
    lv_anim_set_exec_cb(&a, buoy_translate_exec);
    lv_anim_set_values(&a, from_translate, 0);
    lv_anim_set_duration(&a, UI_BUOY_RELEASE_DURATION);
    lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
    lv_anim_start(&a);
}

/**
 * 浮标统一事件回调
 *
 * 生物拟态交互核心：
 *   PRESSED  → 记录起始，微缩按压反馈
 *   PRESSING → 弹性跟随 (阻尼 0.7)，超阈值自动展开/折叠
 *   RELEASED→ 松手回弹或 snap
 *   CLICKED → 点击展开浮标 / 切换页面
 */
static void buoy_event_cb(lv_event_t *e)
{
    auto *shell = static_cast<LaserUiShell *>(lv_event_get_user_data(e));
    if (shell == nullptr) { return; }

    lv_obj_t *buoy = static_cast<lv_obj_t *>(lv_event_get_target(e));
    const lv_event_code_t code = lv_event_get_code(e);
    int page_i = laser_ui_buoy_get_page_index(buoy);
    if (page_i < 0 || page_i >= static_cast<int>(LaserPage::Count)) { return; }

    const bool is_active_page =
        shell->page_open && static_cast<int>(shell->current) == page_i;

    switch (code) {

    // ================================================================
    // PRESSED: 按压反馈 — 微缩 + 增亮光晕
    // ================================================================
    case LV_EVENT_PRESSED: {
        int py = 0;
        pointer_local_point(shell, &shell->nav_drag_origin_x, &py);
        shell->nav_drag_origin_x = pointer_x_in_root(shell);
        shell->nav_drag_page = page_i;
        shell->nav_drag_snapped = false;
        shell->nav_suppress_click = false;
        shell->nav_drag_follow = 0;

        lv_obj_set_style_transform_scale(buoy, UI_BUOY_PRESS_SCALE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(buoy, UI_COLOR_BUOY_PRESS, LV_PART_MAIN);
        lv_obj_set_style_outline_opa(buoy, LV_OPA_60, LV_PART_MAIN);

        /* 按压时暂停呼吸动画 */
        laser_ui_buoy_set_breathing(buoy, false);
        break;
    }

    // ================================================================
    // PRESSING: 弹性拖拽 — 阻尼跟随 + 拉伸变形 + 自动 snap
    // ================================================================
    case LV_EVENT_PRESSING: {
        if (shell->nav_drag_snapped) { break; }

        const int dx = pointer_x_in_root(shell) - shell->nav_drag_origin_x;
        int follow = (int)((float)dx * UI_BUOY_DRAG_DAMPING);
        follow = clamp_int(follow, -UI_BUOY_DRAG_MAX_FOLLOW, UI_BUOY_DRAG_MAX_FOLLOW);
        shell->nav_drag_follow = follow;

        /* 折叠态: 允许水平和垂直方向拖拽 */
        if (!shell->buoys_expanded) {
            /* 水平方向约束: 右拖展开，左拖无效 */
            int h_follow = (follow < 0) ? 0 : follow;
            lv_obj_set_style_translate_x(buoy, h_follow, LV_PART_MAIN);

            /* 垂直方向: 允许在 Y 范围内自由移动 */
            int py = 0;
            pointer_local_point(shell, nullptr, &py);
            int buoy_half = UI_BUOY_R;
            int y_min = UI_TOP_H + buoy_half + 4;
            int y_max = UI_TOP_H + UI_MAIN_H - buoy_half - 4;
            int clamped_y = clamp_int(py, y_min, y_max);
            lv_obj_set_style_translate_y(buoy, clamped_y - lv_obj_get_y(buoy) - buoy_half, LV_PART_MAIN);

            /* 弹性形变 */
            int abs_f = (follow < 0) ? -follow : follow;
            int sx = 1000 - abs_f * 3;
            int sy = 1000 + abs_f * 2;
            if (sx < 850) { sx = 850; }
            if (sy > 1080) { sy = 1080; }
            lv_obj_set_style_transform_scale_x(buoy, sx, LV_PART_MAIN);
            lv_obj_set_style_transform_scale_y(buoy, sy, LV_PART_MAIN);

            /* 水平拖拽超过阈值 → 自动展开浮标群 */
            if (dx >= UI_BUOY_DRAG_OPEN_PX) {
                shell_expand_buoys(shell);
                shell->nav_drag_snapped = true;
                shell->nav_suppress_click = true;
                lv_obj_set_style_translate_x(buoy, 0, LV_PART_MAIN);
                lv_obj_set_style_translate_y(buoy, 0, LV_PART_MAIN);
                lv_obj_set_style_transform_scale_x(buoy, 1000, LV_PART_MAIN);
                lv_obj_set_style_transform_scale_y(buoy, 1000, LV_PART_MAIN);
            }
        } else {
            /* 展开态: 仅水平方向拖拽 */
            /* 方向约束: 非活动页只能右拖（无操作），活动页只能左拖（折叠） */
            if (!is_active_page && follow < 0) { follow = 0; }
            if (is_active_page && follow > 0) { follow = 0; }

            lv_obj_set_style_translate_x(buoy, follow, LV_PART_MAIN);

            /* 弹性形变 */
            int abs_f = (follow < 0) ? -follow : follow;
            int sx = 1000 - abs_f * 3;
            int sy = 1000 + abs_f * 2;
            if (sx < 850) { sx = 850; }
            if (sy > 1080) { sy = 1080; }
            lv_obj_set_style_transform_scale_x(buoy, sx, LV_PART_MAIN);
            lv_obj_set_style_transform_scale_y(buoy, sy, LV_PART_MAIN);

            /* 左拖超过阈值 → 折叠 */
            if (dx <= -UI_BUOY_DRAG_CLOSE_PX && is_active_page) {
                shell_collapse_buoys(shell);
                shell->nav_drag_snapped = true;
                shell->nav_suppress_click = true;
                lv_obj_set_style_translate_x(buoy, 0, LV_PART_MAIN);
                lv_obj_set_style_transform_scale_x(buoy, 1000, LV_PART_MAIN);
                lv_obj_set_style_transform_scale_y(buoy, 1000, LV_PART_MAIN);
            }
        }
        break;
    }

    // ================================================================
    // RELEASED: 松手 — 恢复变形 + 回弹 / snap
    // ================================================================
    case LV_EVENT_RELEASED: {
        /* 恢复默认外观 */
        lv_obj_set_style_transform_scale(buoy, 1000, LV_PART_MAIN);
        lv_obj_set_style_bg_color(buoy,
            is_active_page ? UI_COLOR_BUOY_ACTIVE : UI_COLOR_BUOY_IDLE, LV_PART_MAIN);
        lv_obj_set_style_outline_opa(buoy,
            is_active_page ? LV_OPA_50 : LV_OPA_30, LV_PART_MAIN);
        lv_obj_set_style_transform_scale_x(buoy, 1000, LV_PART_MAIN);
        lv_obj_set_style_transform_scale_y(buoy, 1000, LV_PART_MAIN);

        if (!shell->nav_drag_snapped) {
            const int dx = pointer_x_in_root(shell) - shell->nav_drag_origin_x;

            if (dx >= UI_BUOY_DRAG_RELEASE_OPEN_PX && !shell->buoys_expanded) {
                /* 右拖超过阈值 → 展开浮标群 */
                shell_expand_buoys(shell);
                shell->nav_suppress_click = true;
            } else if (dx <= -UI_BUOY_DRAG_RELEASE_CLOSE_PX && is_active_page &&
                       shell->buoys_expanded) {
                /* 左拖超过阈值 → 折叠 */
                shell_collapse_buoys(shell);
                shell->nav_suppress_click = true;
            } else if (abs(dx) > 4 || shell->nav_drag_follow != 0) {
                /* 小拖拽 → 回弹 */
                if (shell->nav_drag_follow != 0) {
                    lv_anim_del(buoy, buoy_translate_exec);
                    buoy_spring_back(buoy, shell->nav_drag_follow);
                }
                /* 垂直位置回弹 */
                lv_obj_set_style_translate_y(buoy, 0, LV_PART_MAIN);
                if (abs(dx) > 4) {
                    shell->nav_suppress_click = true;
                }
            }
        }

        shell->nav_drag_page = -1;
        shell->nav_drag_snapped = false;
        shell->nav_drag_follow = 0;

        /* 折叠态恢复呼吸 */
        if (!shell->buoys_expanded && is_active_page) {
            laser_ui_buoy_set_breathing(buoy, true);
        }
        break;
    }

    // ================================================================
    // CLICKED: 短点击 — 折叠态展开，展开态切页+折叠
    // ================================================================
    case LV_EVENT_CLICKED: {
        if (shell->nav_suppress_click) {
            shell->nav_suppress_click = false;
            break;
        }

        if (!shell->buoys_expanded) {
            /* 折叠态 → 展开浮标群 */
            shell_expand_buoys(shell);
        } else {
            if (is_active_page) {
                /* 点击当前页面浮标 → 仅折叠 */
                shell_collapse_buoys(shell);
            } else {
                /* 点击其他页面浮标 → 切页 + 折叠 */
                shell_open_page(shell, static_cast<LaserPage>(page_i), true);
                shell_collapse_buoys(shell);
            }
        }
        break;
    }

    default:
        break;
    }
}

// ============================================================================
//  公开 API — Public interface
// ============================================================================

void laser_ui_shell_switch_page(LaserUiShell *shell, LaserPage page)
{
    shell_open_page(shell, page, false);
}

void laser_ui_shell_init(LaserUiShell *shell, lv_obj_t *screen)
{
    if (shell == nullptr || screen == nullptr) { return; }

    laser_ui_apply_main_gradient(screen);

    /* ---- 1. 根容器 ---- */
    shell->root = lv_obj_create(screen);
    lv_obj_set_size(shell->root, LV_HOR_RES, UI_MAIN_H);
    lv_obj_align(shell->root, LV_ALIGN_TOP_MID, 0, UI_TOP_H);
    lv_obj_set_style_bg_opa(shell->root, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(shell->root, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(shell->root, 0, LV_PART_MAIN);
    lv_obj_clear_flag(shell->root, LV_OBJ_FLAG_SCROLLABLE);
    laser_ui_add_background_pattern(shell->root);

    /* ---- 2. 内容宿主 ---- */
    shell->content_host = lv_obj_create(shell->root);
    lv_obj_set_size(shell->content_host, UI_CONTENT_W, LV_PCT(100));
    lv_obj_set_pos(shell->content_host, UI_NAV_EXPANDED_W, 0);
    lv_obj_set_style_bg_opa(shell->content_host, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(shell->content_host, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(shell->content_host, 0, LV_PART_MAIN);
    lv_obj_add_flag(shell->content_host, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_clear_flag(shell->content_host, LV_OBJ_FLAG_SCROLLABLE);

    /* ---- 3. 创建三个页面 ---- */
    shell->pages[static_cast<int>(LaserPage::Print)] =
        page_print_create(shell->content_host);
    shell->pages[static_cast<int>(LaserPage::Settings)] =
        page_settings_create(shell->content_host);
    shell->pages[static_cast<int>(LaserPage::Pick)] =
        page_pick_create(shell->content_host);

    for (int i = 0; i < static_cast<int>(LaserPage::Count); ++i) {
        if (shell->pages[i] != nullptr) {
            lv_obj_set_size(shell->pages[i], UI_CONTENT_W, LV_PCT(100));
            lv_obj_add_flag(shell->pages[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    /* ---- 4. 浮标层 ---- */
    shell->buoy_layer = lv_obj_create(shell->root);
    lv_obj_set_size(shell->buoy_layer, LV_HOR_RES, LV_PCT(100));
    lv_obj_set_pos(shell->buoy_layer, 0, 0);
    lv_obj_set_style_bg_opa(shell->buoy_layer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(shell->buoy_layer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(shell->buoy_layer, 0, LV_PART_MAIN);
    lv_obj_clear_flag(shell->buoy_layer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(shell->buoy_layer, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_user_data(shell->buoy_layer, shell);

    /* ---- 5. 创建三个生物拟态浮标 ---- */
    struct NavItem { const char *icon; };
    const NavItem nav_items[] = {
        {LV_SYMBOL_PLAY},
        {LV_SYMBOL_SETTINGS},
        {LV_SYMBOL_EDIT},
    };

    int y_centers[UI_BUOY_COUNT];
    calc_buoy_expanded_positions(y_centers);
    const int collapsed_y = y_centers[1];  // 折叠态用中间浮标的位置

    for (int i = 0; i < static_cast<int>(LaserPage::Count); ++i) {
        shell->buoys[i] = laser_ui_create_buoy(shell->buoy_layer,
                                                nav_items[i].icon, i);
        if (shell->buoys[i] != nullptr) {
            lv_obj_set_pos(shell->buoys[i],
                           UI_BUOY_COLLAPSED_X - UI_BUOY_R,
                           collapsed_y - UI_BUOY_R);
            lv_obj_add_event_cb(shell->buoys[i], buoy_event_cb,
                                LV_EVENT_PRESSED, shell);
            lv_obj_add_event_cb(shell->buoys[i], buoy_event_cb,
                                LV_EVENT_PRESSING, shell);
            lv_obj_add_event_cb(shell->buoys[i], buoy_event_cb,
                                LV_EVENT_RELEASED, shell);
            lv_obj_add_event_cb(shell->buoys[i], buoy_event_cb,
                                LV_EVENT_CLICKED, shell);
            lv_obj_add_flag(shell->buoys[i], LV_OBJ_FLAG_CLICKABLE);
        }
    }

    /* 初始折叠态: 只显示 Print 浮标 */
    shell->buoys_expanded = false;
    if (shell->buoys[0] != nullptr) {
        lv_obj_remove_flag(shell->buoys[0], LV_OBJ_FLAG_HIDDEN);
        laser_ui_buoy_set_active(shell->buoys[0], true);
        laser_ui_buoy_set_breathing(shell->buoys[0], true);
    }
    for (int i = 1; i < static_cast<int>(LaserPage::Count); ++i) {
        if (shell->buoys[i] != nullptr) {
            lv_obj_add_flag(shell->buoys[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    /* ---- 6. 启动 ---- */
    shell_raise_buoy_layer(shell);
    shell_open_page(shell, LaserPage::Print, false);
}
