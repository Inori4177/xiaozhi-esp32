#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Store VFS path (e.g. /localfs/foo.gcode) selected from WebUI preview. */
void ui_gcode_preview_set_path(const char *vfs_path);

/** Human-readable basename of current preview path, or empty string. */
const char *ui_gcode_preview_get_path(void);

/** Cancel in-flight background render (call from page hide). */
void ui_gcode_preview_cancel(void);

/**
 * Reload preview for current path into canvas; optional labels updated on completion.
 * Non-blocking: parses on Core 0 worker task.
 */
void ui_gcode_preview_refresh(lv_obj_t *canvas, lv_obj_t *name_label, lv_obj_t *status_label);

/** One-time buffer init; safe to call multiple times. */
void ui_gcode_preview_init(void);

/** Attach shared RGB565 buffer to canvas (call once after lv_canvas_create). */
void ui_gcode_preview_bind_canvas(lv_obj_t *canvas);

/** Register Print page widgets; enables live refresh when WebUI POST /preview. */
void ui_gcode_preview_bind_ui(lv_obj_t *canvas, lv_obj_t *name_label, lv_obj_t *status_label);

/** Clear Print page widget registration (call from page hide). */
void ui_gcode_preview_unbind_ui(void);

/** Canvas pixel width/height used on Print page. */
#define UI_GCODE_PREVIEW_CANVAS_PX 168

#ifdef __cplusplus
}
#endif
