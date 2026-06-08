#pragma once

#include <lvgl.h>

// ============================================================================
// 通用面板 / 按钮样式
// ============================================================================
lv_obj_t *laser_ui_apply_panel_style(lv_obj_t *obj, lv_color_t bg, int pad);
void laser_ui_apply_button_style(lv_obj_t *btn, lv_color_t bg, lv_color_t pressed_bg);
lv_obj_t *laser_ui_create_button(lv_obj_t *parent, const char *text, lv_color_t bg, lv_color_t pressed_bg);
lv_obj_t *laser_ui_create_tab_button(lv_obj_t *parent, const char *text, bool active);
lv_obj_t *laser_ui_create_hud_panel(lv_obj_t *parent, lv_color_t bg, int pad);
void laser_ui_add_corner_marks(lv_obj_t *parent, lv_color_t color);
void laser_ui_style_energy_slider(lv_obj_t *slider, lv_color_t color);
void laser_ui_style_status_bar(lv_obj_t *bar, lv_color_t color);
void laser_ui_add_map_grid(lv_obj_t *parent, lv_color_t color);
void laser_ui_add_background_pattern(lv_obj_t *parent);
void laser_ui_add_panel_scanline(lv_obj_t *parent, lv_color_t color);
lv_obj_t *laser_ui_create_image_button(lv_obj_t *parent, const lv_image_dsc_t *src,
                                       const char *text, lv_color_t text_color);
lv_obj_t *laser_ui_create_png_button(lv_obj_t *parent, const lv_image_dsc_t *src,
                                     int w, int h, int radius, int ext_click);
void laser_ui_add_title_icon(lv_obj_t *parent, const lv_image_dsc_t *src,
                             const char *text, lv_color_t color);

// ============================================================================
// 生物拟态导航浮标 — Biomimetic buoy widget
// ============================================================================

/**
 * 创建一个可交互的圆形导航浮标
 * @param parent  父容器
 * @param icon    LVGL 图标字符 (e.g. LV_SYMBOL_PLAY)
 * @param page_index  页面索引 (0=Print, 1=Settings, 2=Pick)
 * @return 浮标对象指针
 *
 * 特性：
 *   - 呼吸感动画（正弦波振幅/频率模拟水母）
 *   - 按压形变反馈
 *   - 拖拽弹性跟随
 */
lv_obj_t *laser_ui_create_buoy(lv_obj_t *parent, const char *icon, int page_index);

/**
 * 设置浮标的展开/折叠状态（动画过渡）
 * @param buoy     浮标对象
 * @param expanded 是否展开态
 * @param delay_ms 动画延迟（用于错开效果）
 * @param target_x 展开态 X 坐标
 * @param target_y 展开态 Y 坐标
 */
void laser_ui_buoy_set_expanded(lv_obj_t *buoy, bool expanded, int delay_ms,
                                int target_x, int target_y);

/**
 * 设置浮标的激活态（当前页面高亮）
 */
void laser_ui_buoy_set_active(lv_obj_t *buoy, bool active);

/**
 * 启动/停止浮标的呼吸动画
 */
void laser_ui_buoy_set_breathing(lv_obj_t *buoy, bool enable);

/**
 * 获取浮标绑定的页面索引
 */
int laser_ui_buoy_get_page_index(lv_obj_t *buoy);
