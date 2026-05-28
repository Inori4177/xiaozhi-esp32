#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int area_x;
    int area_y;
    int area_w;
    int area_h;
    float scale; /* pixels per mm */
} ui_cnc_coord_viewport_t;

void ui_cnc_coord_viewport_init(ui_cnc_coord_viewport_t *vp, int host_w, int host_h, int pad_px);

void ui_cnc_mm_to_px(const ui_cnc_coord_viewport_t *vp, float mm_x, float mm_y, int *px, int *py);

void ui_cnc_px_to_mm(const ui_cnc_coord_viewport_t *vp, int px, int py, float *mm_x, float *mm_y);

void ui_cnc_clamp_mm(float *mm_x, float *mm_y);

#ifdef __cplusplus
}
#endif
