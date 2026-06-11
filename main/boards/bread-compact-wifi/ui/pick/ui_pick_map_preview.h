#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

void ui_pick_map_preview_init(void);

/** Create transparent overlay canvas inside map_frame (between grid and head dot). */
lv_obj_t *ui_pick_map_preview_create_overlay(lv_obj_t *map_frame);

void ui_pick_map_preview_on_map_resized(int side_px);
void ui_pick_map_preview_on_page_show(void);
void ui_pick_map_preview_on_page_hide(void);

/** Static preview from WebUI POST /preview or manual path. */
void ui_pick_map_preview_load_path(const char *vfs_path);

/** Run mode: full green toolpath, progress turns segments black. */
void ui_pick_map_preview_start_run(const char *vfs_path);

/** Legacy hook (progress now driven by file run %). */
void ui_pick_map_preview_on_position_mm(float x_mm, float y_mm);

/** Update green→black coverage from G-code file send progress (0–100). */
void ui_pick_map_preview_on_job_progress(uint8_t progress_pct);

/** Hide overlay and reset to idle. */
void ui_pick_map_preview_clear(void);

/** Re-rasterize pending green if static preview is active (e.g. power changed). */
void ui_pick_map_preview_refresh_colors(void);

/** Re-apply pick-origin offset after origin confirm/reset (screen or WebUI). */
void ui_pick_map_preview_on_origin_changed(void);

#ifdef __cplusplus
}
#endif
