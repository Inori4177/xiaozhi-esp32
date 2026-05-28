#include "ui_cnc_coord_map.h"

#include "ui_cnc_config.h"

#include <algorithm>
#include <cmath>

void ui_cnc_coord_viewport_init(ui_cnc_coord_viewport_t *vp, int host_w, int host_h, int pad_px)
{
    if (vp == nullptr) {
        return;
    }

    const int inner_w = host_w - pad_px * 2;
    const int inner_h = host_h - pad_px * 2;
    const int side = std::min(inner_w, inner_h);

    vp->area_w = side;
    vp->area_h = side;
    vp->area_x = pad_px + (inner_w - side) / 2;
    vp->area_y = pad_px + (inner_h - side) / 2;
    vp->scale = static_cast<float>(side) / UI_CNC_WORK_SIZE_MM;
}

void ui_cnc_mm_to_px(const ui_cnc_coord_viewport_t *vp, float mm_x, float mm_y, int *px, int *py)
{
    if (vp == nullptr || px == nullptr || py == nullptr) {
        return;
    }
    float cx = mm_x;
    float cy = mm_y;
    ui_cnc_clamp_mm(&cx, &cy);

    *px = vp->area_x + static_cast<int>(std::lround(cx * vp->scale));
    /* G-code Y grows upward; screen Y grows downward */
    *py = vp->area_y + vp->area_h - static_cast<int>(std::lround(cy * vp->scale));
}

void ui_cnc_px_to_mm(const ui_cnc_coord_viewport_t *vp, int px, int py, float *mm_x, float *mm_y)
{
    if (vp == nullptr || mm_x == nullptr || mm_y == nullptr || vp->scale <= 0.0f) {
        return;
    }

    const float rel_x = static_cast<float>(px - vp->area_x);
    const float rel_y = static_cast<float>(vp->area_y + vp->area_h - py);

    *mm_x = rel_x / vp->scale;
    *mm_y = rel_y / vp->scale;
    ui_cnc_clamp_mm(mm_x, mm_y);
}

void ui_cnc_clamp_mm(float *mm_x, float *mm_y)
{
    if (mm_x != nullptr) {
        if (*mm_x < 0.0f) {
            *mm_x = 0.0f;
        }
        if (*mm_x > UI_CNC_WORK_SIZE_MM) {
            *mm_x = UI_CNC_WORK_SIZE_MM;
        }
    }
    if (mm_y != nullptr) {
        if (*mm_y < 0.0f) {
            *mm_y = 0.0f;
        }
        if (*mm_y > UI_CNC_WORK_SIZE_MM) {
            *mm_y = UI_CNC_WORK_SIZE_MM;
        }
    }
}
