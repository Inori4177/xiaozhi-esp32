#include "laser_ui_widgets.h"
#include "laser_ui_layout.h"
#include "assets/laser_ui_images.h"

#include <math.h>
#include <stdlib.h>

// ============================================================================
// 通用面板样式 — 圆角卡片 + 微光泽边框
// ============================================================================
lv_obj_t *laser_ui_apply_panel_style(lv_obj_t *obj, lv_color_t bg, int pad)
{
    /* 圆角 6px — 工业 HUD 风格 */
    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj, bg, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
    /* 微细边框，低透明度营造深度感 */
    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(obj, UI_COLOR_LIGHT_BORDER, LV_PART_MAIN);
    lv_obj_set_style_border_opa(obj, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, pad, LV_PART_MAIN);
    /* 无阴影—LVGL 嵌入式端 shadow 性能较高，如需可用轻微 shadow 替代 */
    lv_obj_set_style_shadow_width(obj, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(obj, lv_color_hex(0x9ED8CF), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(obj, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_x(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(obj, 4, LV_PART_MAIN);
    return obj;
}

// ============================================================================
// 通用按钮样式 — 圆角药丸型，按压缩放反馈
// ============================================================================
void laser_ui_apply_button_style(lv_obj_t *btn, lv_color_t bg, lv_color_t pressed_bg)
{
    /* 药丸形圆角 */
    lv_obj_set_style_radius(btn, 12, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, pressed_bg, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, UI_COLOR_LIGHT_BORDER, LV_PART_MAIN);
    lv_obj_set_style_border_opa(btn, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 7, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(btn, lv_color_hex(0xB9DCD6), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(btn, 3, LV_PART_MAIN);
    lv_obj_set_style_text_color(btn, UI_COLOR_TEXT, LV_PART_MAIN);
    /* 按压时整体缩小 4%，营造"按下去"的触感 */
    lv_obj_set_style_transform_width(btn, -2, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_transform_height(btn, -2, LV_PART_MAIN | LV_STATE_PRESSED);
    /* 扩展触控热区 */
    lv_obj_set_ext_click_area(btn, 8);
}

lv_obj_t *laser_ui_create_button(lv_obj_t *parent, const char *text, lv_color_t bg, lv_color_t pressed_bg)
{
    lv_obj_t *btn = lv_button_create(parent);
    laser_ui_apply_button_style(btn, bg, pressed_bg);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    return btn;
}

lv_obj_t *laser_ui_create_tab_button(lv_obj_t *parent, const char *text, bool active)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_height(btn, 26);
    lv_obj_set_style_pad_hor(btn, 10, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 6, LV_PART_MAIN);
    lv_color_t bg = active ? UI_COLOR_SOFT_MINT : UI_COLOR_SURFACE;
    lv_color_t pressed = active ? UI_COLOR_MINT : UI_COLOR_CARD_ALT;
    laser_ui_apply_button_style(btn, bg, pressed);
    lv_obj_set_style_text_color(btn, active ? UI_COLOR_TEXT : UI_COLOR_TEXT_SEC, LV_PART_MAIN);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_center(lbl);
    return btn;
}

lv_obj_t *laser_ui_create_image_button(lv_obj_t *parent, const lv_image_dsc_t *src,
                                       const char *text, lv_color_t text_color)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, LV_OPA_20, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(btn, lv_color_white(), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);
    lv_obj_set_ext_click_area(btn, 8);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    if (src != nullptr) {
        lv_obj_t *img = lv_image_create(btn);
        lv_image_set_src(img, src);
        lv_obj_center(img);
        lv_obj_remove_flag(img, LV_OBJ_FLAG_CLICKABLE);
    }

    if (text != nullptr && text[0] != '\0') {
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, text);
        lv_obj_set_style_text_color(label, text_color, LV_PART_MAIN);
        lv_obj_center(label);
        lv_obj_remove_flag(label, LV_OBJ_FLAG_CLICKABLE);
    }
    return btn;
}

void laser_ui_add_title_icon(lv_obj_t *parent, const lv_image_dsc_t *src,
                             const char *text, lv_color_t color)
{
    if (parent == nullptr) {
        return;
    }
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), 24);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(row, 3, LV_PART_MAIN);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    if (src != nullptr) {
        lv_obj_t *img = lv_image_create(row);
        lv_image_set_src(img, src);
        lv_obj_set_size(img, 20, 20);
        lv_obj_remove_flag(img, LV_OBJ_FLAG_CLICKABLE);
    }

    lv_obj_t *label = lv_label_create(row);
    lv_label_set_text(label, text != nullptr ? text : "");
    lv_obj_set_style_text_color(label, color, LV_PART_MAIN);
}

static lv_obj_t *laser_ui_create_deco_rect(lv_obj_t *parent, int w, int h,
                                           lv_color_t color, lv_opa_t opa)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_bg_color(obj, color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, opa, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_FLOATING);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

void laser_ui_add_corner_marks(lv_obj_t *parent, lv_color_t color)
{
    if (parent == nullptr) {
        return;
    }

    const int len = 14;
    const int thick = 2;
    const int ofs = 3;
    const lv_opa_t opa = LV_OPA_70;

    lv_obj_t *tl_h = laser_ui_create_deco_rect(parent, len, thick, color, opa);
    lv_obj_align(tl_h, LV_ALIGN_TOP_LEFT, ofs, ofs);
    lv_obj_t *tl_v = laser_ui_create_deco_rect(parent, thick, len, color, opa);
    lv_obj_align(tl_v, LV_ALIGN_TOP_LEFT, ofs, ofs);

    lv_obj_t *tr_h = laser_ui_create_deco_rect(parent, len, thick, color, opa);
    lv_obj_align(tr_h, LV_ALIGN_TOP_RIGHT, -ofs, ofs);
    lv_obj_t *tr_v = laser_ui_create_deco_rect(parent, thick, len, color, opa);
    lv_obj_align(tr_v, LV_ALIGN_TOP_RIGHT, -ofs, ofs);

    lv_obj_t *bl_h = laser_ui_create_deco_rect(parent, len, thick, color, opa);
    lv_obj_align(bl_h, LV_ALIGN_BOTTOM_LEFT, ofs, -ofs);
    lv_obj_t *bl_v = laser_ui_create_deco_rect(parent, thick, len, color, opa);
    lv_obj_align(bl_v, LV_ALIGN_BOTTOM_LEFT, ofs, -ofs);

    lv_obj_t *br_h = laser_ui_create_deco_rect(parent, len, thick, color, opa);
    lv_obj_align(br_h, LV_ALIGN_BOTTOM_RIGHT, -ofs, -ofs);
    lv_obj_t *br_v = laser_ui_create_deco_rect(parent, thick, len, color, opa);
    lv_obj_align(br_v, LV_ALIGN_BOTTOM_RIGHT, -ofs, -ofs);
}

lv_obj_t *laser_ui_create_hud_panel(lv_obj_t *parent, lv_color_t bg, int pad)
{
    lv_obj_t *panel = lv_obj_create(parent);
    laser_ui_apply_panel_style(panel, bg, pad);
    lv_obj_set_style_border_color(panel, UI_COLOR_LIGHT_BORDER, LV_PART_MAIN);
    lv_obj_set_style_border_opa(panel, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_outline_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_outline_color(panel, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_outline_opa(panel, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_outline_pad(panel, 1, LV_PART_MAIN);
    return panel;
}

void laser_ui_style_energy_slider(lv_obj_t *slider, lv_color_t color)
{
    if (slider == nullptr) {
        return;
    }

    lv_obj_set_style_bg_color(slider, lv_color_hex(0xEDF5F3), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(slider, 4, LV_PART_MAIN);
    lv_obj_set_style_border_width(slider, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(slider, UI_COLOR_LIGHT_BORDER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, color, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(slider, 4, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_white(), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_border_width(slider, 2, LV_PART_KNOB);
    lv_obj_set_style_border_color(slider, color, LV_PART_KNOB);
    lv_obj_set_style_shadow_width(slider, 8, LV_PART_KNOB);
    lv_obj_set_style_shadow_color(slider, color, LV_PART_KNOB);
    lv_obj_set_style_shadow_opa(slider, LV_OPA_40, LV_PART_KNOB);
}

void laser_ui_style_status_bar(lv_obj_t *bar, lv_color_t color)
{
    if (bar == nullptr) {
        return;
    }

    lv_obj_set_style_bg_color(bar, lv_color_hex(0xEEF6F3), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, color, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 3, LV_PART_INDICATOR);
    lv_obj_set_style_border_width(bar, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(bar, UI_COLOR_LIGHT_BORDER, LV_PART_MAIN);
}

void laser_ui_add_map_grid(lv_obj_t *parent, lv_color_t color)
{
    if (parent == nullptr) {
        return;
    }

    const int offsets[] = { -96, -64, -32, 0, 32, 64, 96 };
    const int count = sizeof(offsets) / sizeof(offsets[0]);

    for (int i = 0; i < count; ++i) {
        lv_opa_t opa = (offsets[i] == 0) ? LV_OPA_40 : LV_OPA_20;

        lv_obj_t *v = laser_ui_create_deco_rect(parent, 1, LV_PCT(100), color, opa);
        lv_obj_align(v, LV_ALIGN_CENTER, offsets[i], 0);

        lv_obj_t *h = laser_ui_create_deco_rect(parent, LV_PCT(100), 1, color, opa);
        lv_obj_align(h, LV_ALIGN_CENTER, 0, offsets[i]);
    }

    lv_obj_t *mid_dot = laser_ui_create_deco_rect(parent, 6, 6, color, LV_OPA_60);
    lv_obj_set_style_radius(mid_dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_center(mid_dot);

    const int tick_offsets[] = { -72, -36, 36, 72 };
    const int tick_count = sizeof(tick_offsets) / sizeof(tick_offsets[0]);
    for (int i = 0; i < tick_count; ++i) {
        lv_obj_t *top = laser_ui_create_deco_rect(parent, 10, 2, color, LV_OPA_40);
        lv_obj_align(top, LV_ALIGN_TOP_MID, tick_offsets[i], 4);
        lv_obj_t *bottom = laser_ui_create_deco_rect(parent, 10, 2, color, LV_OPA_40);
        lv_obj_align(bottom, LV_ALIGN_BOTTOM_MID, tick_offsets[i], -4);
        lv_obj_t *left = laser_ui_create_deco_rect(parent, 2, 10, color, LV_OPA_40);
        lv_obj_align(left, LV_ALIGN_LEFT_MID, 4, tick_offsets[i]);
        lv_obj_t *right = laser_ui_create_deco_rect(parent, 2, 10, color, LV_OPA_40);
        lv_obj_align(right, LV_ALIGN_RIGHT_MID, -4, tick_offsets[i]);
    }
}

void laser_ui_add_background_pattern(lv_obj_t *parent)
{
    if (parent == nullptr) {
        return;
    }

    lv_obj_t *bg = lv_image_create(parent);
    lv_image_set_src(bg, &fresh_light_background);
    lv_obj_align(bg, LV_ALIGN_TOP_LEFT, 0, -UI_TOP_H);
    lv_obj_move_background(bg);
    lv_obj_remove_flag(bg, LV_OBJ_FLAG_CLICKABLE);
}

void laser_ui_add_panel_scanline(lv_obj_t *parent, lv_color_t color)
{
    if (parent == nullptr) {
        return;
    }

    lv_obj_t *line = laser_ui_create_deco_rect(parent, LV_PCT(100), 1, color, LV_OPA_20);
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, 20);
}

// ============================================================================
//  生物拟态导航浮标实现 — Biomimetic Navigation Buoy
// ============================================================================

/**
 * 浮标内部状态 — 挂载在 buoy 对象上，通过 lv_obj_get/set_user_data 存取
 */
struct BuoyState {
    int  page_index;        // 绑定的页面索引
    bool breathing;         // 呼吸动画是否开启
    float breath_phase;     // 呼吸动画相位 (radians)
    lv_timer_t *breath_timer; // 呼吸定时器句柄
};

/* 向前声明 */
/* ---------- 获取浮标内部状态 ---------- */
static BuoyState *buoy_get_state(lv_obj_t *buoy)
{
    return static_cast<BuoyState *>(lv_obj_get_user_data(buoy));
}

int laser_ui_buoy_get_page_index(lv_obj_t *buoy)
{
    BuoyState *st = buoy_get_state(buoy);
    return st ? st->page_index : -1;
}

/* ---------- 浮标删除回调 — 自动释放 BuoyState ---------- */
static void buoy_delete_cb(lv_event_t *e)
{
    lv_obj_t *obj = static_cast<lv_obj_t *>(lv_event_get_target(e));
    if (obj == nullptr) { return; }
    BuoyState *state = static_cast<BuoyState *>(lv_obj_get_user_data(obj));
    if (state != nullptr) {
        lv_free(state);
        lv_obj_set_user_data(obj, nullptr);
    }
}

/* ---------- 创建浮标 ---------- */
lv_obj_t *laser_ui_create_buoy(lv_obj_t *parent, const char *icon, int page_index)
{
    /* 以 lv_obj 而非 lv_button 创建，完全自定义交互 */
    lv_obj_t *buoy = lv_obj_create(parent);
    int size = UI_BUOY_R * 2;
    lv_obj_set_size(buoy, size, size);

    /* --- 圆形外观 (OLED 暗色主题) --- */
    lv_obj_set_style_radius(buoy, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(buoy, UI_COLOR_BUOY_IDLE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(buoy, LV_OPA_90, LV_PART_MAIN);
    lv_obj_set_style_border_width(buoy, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(buoy, lv_color_hex(0x3D5A90), LV_PART_MAIN);
    lv_obj_set_style_border_opa(buoy, LV_OPA_70, LV_PART_MAIN);

    /* 发光外描边 (outline) — 模拟浮标在水中的光晕, 随尺寸增大而增强 */
    lv_obj_set_style_outline_width(buoy, 3, LV_PART_MAIN);
    lv_obj_set_style_outline_color(buoy, UI_COLOR_BUOY_GLOW, LV_PART_MAIN);
    lv_obj_set_style_outline_opa(buoy, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_outline_pad(buoy, 2, LV_PART_MAIN);

    /* 轻微阴影增强立体感 */
    lv_obj_set_style_shadow_width(buoy, 4, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(buoy, UI_COLOR_BUOY_GLOW, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(buoy, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_x(buoy, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(buoy, 2, LV_PART_MAIN);

    /* 无内边距，子对象居中 */
    lv_obj_set_style_pad_all(buoy, 0, LV_PART_MAIN);
    lv_obj_clear_flag(buoy, LV_OBJ_FLAG_SCROLLABLE);

    /* 默认不可见 — 由 shell 层控制展开/折叠动画 */
    lv_obj_add_flag(buoy, LV_OBJ_FLAG_HIDDEN);

    /* --- 图标标签居中 --- */
    lv_obj_t *icon_lbl = lv_label_create(buoy);
    lv_label_set_text(icon_lbl, icon);
    lv_obj_set_style_text_color(icon_lbl, lv_color_hex(0xE8EDF5), LV_PART_MAIN);
    lv_obj_set_style_text_font(icon_lbl, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_center(icon_lbl);
    lv_obj_remove_flag(icon_lbl, LV_OBJ_FLAG_CLICKABLE);

    /* --- 扩展触控热区至 UI_BUOY_HIT_R --- */
    int hit_extra = UI_BUOY_HIT_R - UI_BUOY_R;
    lv_obj_set_ext_click_area(buoy, hit_extra);

    /* --- 分配并挂载内部状态 --- */
    BuoyState *st = static_cast<BuoyState *>(lv_malloc(sizeof(BuoyState)));
    if (st == nullptr) {
        return buoy;  // 内存不足，返回无状态的浮标
    }
    st->page_index   = page_index;
    st->breathing    = false;
    st->breath_phase = 0.0f;
    st->breath_timer = nullptr;
    lv_obj_set_user_data(buoy, st);

    /* 注册删除回调 — 浮标销毁时自动释放 BuoyState，防止内存泄漏 */
    lv_obj_add_event_cb(buoy, buoy_delete_cb, LV_EVENT_DELETE, nullptr);

    return buoy;
}

/* ---------- 呼吸定时器回调 — lv_timer 驱动，无每帧分配开销 ---------- */
static void buoy_breath_timer_cb(lv_timer_t *timer)
{
    lv_obj_t *buoy = static_cast<lv_obj_t *>(lv_timer_get_user_data(timer));
    if (buoy == nullptr) { return; }
    BuoyState *st = buoy_get_state(buoy);
    if (st == nullptr || !st->breathing) { return; }

    /* 正弦波相位: 0.8Hz 周期约 1.25s，每 25ms 推进 ~0.126 rad */
    st->breath_phase += 0.126f;
    if (st->breath_phase > 2.0f * M_PI) {
        st->breath_phase -= 2.0f * M_PI;
    }

    float sin_val = sinf(st->breath_phase);

    /* 振幅 0.5–1.2px → ±3 scale 单位 (0.3%)，±1.2px 浮动 */
    int32_t scale_delta = (int32_t)(sin_val * 3.0f);
    int32_t translate_delta = (int32_t)(sin_val * 1.2f);

    lv_obj_set_style_transform_scale(buoy, 1000 + scale_delta, LV_PART_MAIN);
    lv_obj_set_style_translate_y(buoy, translate_delta, LV_PART_MAIN);

    /* 光晕脉搏 */
    lv_opa_t glow = (lv_opa_t)(LV_OPA_20 + (int)(sin_val * 10.0f + 10.0f));
    lv_obj_set_style_outline_opa(buoy, glow, LV_PART_MAIN);
}

/* ---------- 启动/停止呼吸动画 ---------- */
void laser_ui_buoy_set_breathing(lv_obj_t *buoy, bool enable)
{
    BuoyState *st = buoy_get_state(buoy);
    if (st == nullptr) { return; }

    /* 停止已有定时器，防止重复叠加 */
    if (st->breath_timer != nullptr) {
        lv_timer_del(st->breath_timer);
        st->breath_timer = nullptr;
    }
    st->breathing = enable;

    if (enable) {
        /* 创建持久定时器，每 25ms 触发一次 (~40fps)，无每帧分配开销 */
        st->breath_timer = lv_timer_create(buoy_breath_timer_cb, 25, buoy);
    } else {
        /* 停止：恢复默认外观 */
        lv_obj_set_style_transform_scale(buoy, 1000, LV_PART_MAIN);
        lv_obj_set_style_translate_y(buoy, 0, LV_PART_MAIN);
        lv_obj_set_style_outline_opa(buoy, LV_OPA_30, LV_PART_MAIN);
    }
}

/* ---------- 设置浮标激活态 — 高亮当前页面浮标 ---------- */
void laser_ui_buoy_set_active(lv_obj_t *buoy, bool active)
{

    if (active) {
        /* 激活态：亮色填充 + 强光晕 + 粗边框 */
        lv_obj_set_style_bg_color(buoy, UI_COLOR_BUOY_ACTIVE, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(buoy, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(buoy, lv_color_hex(0x6DB5FF), LV_PART_MAIN);
        lv_obj_set_style_border_opa(buoy, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_outline_color(buoy, lv_color_hex(0x6DB5FF), LV_PART_MAIN);
        lv_obj_set_style_outline_opa(buoy, LV_OPA_50, LV_PART_MAIN);
    } else {
        /* 非激活态：暗色 + 微光晕 */
        lv_obj_set_style_bg_color(buoy, UI_COLOR_BUOY_IDLE, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(buoy, LV_OPA_90, LV_PART_MAIN);
        lv_obj_set_style_border_color(buoy, lv_color_hex(0x3D5A90), LV_PART_MAIN);
        lv_obj_set_style_border_opa(buoy, LV_OPA_70, LV_PART_MAIN);
        lv_obj_set_style_outline_color(buoy, UI_COLOR_BUOY_GLOW, LV_PART_MAIN);
        lv_obj_set_style_outline_opa(buoy, LV_OPA_30, LV_PART_MAIN);
    }
}

/* ---------- 浮标位置/缩放动画执行回调 ---------- */
static void buoy_pos_exec_cb(void *var, int32_t v)
{
    lv_obj_set_x(static_cast<lv_obj_t *>(var), v);
}

static void buoy_y_exec_cb(void *var, int32_t v)
{
    lv_obj_set_y(static_cast<lv_obj_t *>(var), v);
}

static void buoy_scale_exec_cb(void *var, int32_t v)
{
    lv_obj_set_style_transform_scale(static_cast<lv_obj_t *>(var), v, LV_PART_MAIN);
}

/* 折叠动画完成回调 — 隐藏浮标 */
static void buoy_collapse_ready_cb(lv_anim_t *anim)
{
    if (anim != nullptr && anim->var != nullptr) {
        lv_obj_add_flag(static_cast<lv_obj_t *>(anim->var), LV_OBJ_FLAG_HIDDEN);
    }
}

/* ---------- 展开/折叠动画 ---------- */
void laser_ui_buoy_set_expanded(lv_obj_t *buoy, bool expanded, int delay_ms,
                                int target_x, int target_y)
{
    if (buoy == nullptr) { return; }

    if (expanded) {
        /* 展开：从隐藏淡入 + 从折叠位滑动到展开位 */
        lv_obj_remove_flag(buoy, LV_OBJ_FLAG_HIDDEN);

        /* X 轴滑入 */
        lv_anim_t ax;
        lv_anim_init(&ax);
        lv_anim_set_var(&ax, buoy);
        lv_anim_set_exec_cb(&ax, buoy_pos_exec_cb);
        lv_anim_set_values(&ax, UI_BUOY_COLLAPSED_X - UI_BUOY_R, target_x - UI_BUOY_R);
        lv_anim_set_duration(&ax, UI_BUOY_EXPAND_MS);
        lv_anim_set_delay(&ax, delay_ms);
        lv_anim_set_path_cb(&ax, lv_anim_path_ease_out);
        lv_anim_start(&ax);

        /* Y 轴滑入 — 从目标位置上方 20px 处下落，overshoot 弹跳 */
        lv_anim_t ay;
        lv_anim_init(&ay);
        lv_anim_set_var(&ay, buoy);
        lv_anim_set_exec_cb(&ay, buoy_y_exec_cb);
        lv_anim_set_values(&ay, target_y - UI_BUOY_R - 20, target_y - UI_BUOY_R);
        lv_anim_set_duration(&ay, UI_BUOY_EXPAND_MS);
        lv_anim_set_delay(&ay, delay_ms);
        lv_anim_set_path_cb(&ay, lv_anim_path_overshoot);
        lv_anim_start(&ay);

        /* 缩放: 0.5 → 1.0 (带 overshoot) */
        lv_anim_t as;
        lv_anim_init(&as);
        lv_anim_set_var(&as, buoy);
        lv_anim_set_exec_cb(&as, buoy_scale_exec_cb);
        lv_anim_set_values(&as, 500, 1000);
        lv_anim_set_duration(&as, UI_BUOY_EXPAND_MS);
        lv_anim_set_delay(&as, delay_ms);
        lv_anim_set_path_cb(&as, lv_anim_path_overshoot);
        lv_anim_start(&as);
    } else {
        /* 折叠：缩小并从展开位移到折叠位，最后隐藏 */
        lv_anim_t ax;
        lv_anim_init(&ax);
        lv_anim_set_var(&ax, buoy);
        lv_anim_set_exec_cb(&ax, buoy_pos_exec_cb);
        lv_anim_set_values(&ax, target_x - UI_BUOY_R, UI_BUOY_COLLAPSED_X - UI_BUOY_R);
        lv_anim_set_duration(&ax, UI_BUOY_COLLAPSE_MS);
        lv_anim_set_delay(&ax, delay_ms);
        lv_anim_set_path_cb(&ax, lv_anim_path_ease_in);
        lv_anim_start(&ax);

        /* Y 轴 — 从当前位置滑入折叠位 */
        lv_anim_t ay;
        lv_anim_init(&ay);
        lv_anim_set_var(&ay, buoy);
        lv_anim_set_exec_cb(&ay, buoy_y_exec_cb);
        lv_anim_set_values(&ay, lv_obj_get_y(buoy), target_y - UI_BUOY_R);
        lv_anim_set_duration(&ay, UI_BUOY_COLLAPSE_MS);
        lv_anim_set_delay(&ay, delay_ms);
        lv_anim_start(&ay);

        lv_anim_t as;
        lv_anim_init(&as);
        lv_anim_set_var(&as, buoy);
        lv_anim_set_exec_cb(&as, buoy_scale_exec_cb);
        lv_anim_set_values(&as, 1000, 600);
        lv_anim_set_duration(&as, UI_BUOY_COLLAPSE_MS);
        lv_anim_set_delay(&as, delay_ms);
        lv_anim_set_path_cb(&as, lv_anim_path_ease_in);

        /* 折叠动画完成后隐藏浮标 */
        lv_anim_set_completed_cb(&as, buoy_collapse_ready_cb);
        lv_anim_start(&as);
    }
}
