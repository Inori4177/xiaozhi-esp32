#pragma once

#include <stdint.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const uint8_t splash_bg_gif[];
extern const uint32_t splash_bg_gif_size;

/** Create full-screen looping GIF background on parent; returns image object or nullptr. */
lv_obj_t *splash_bg_gif_start(lv_obj_t *parent);

/** Stop playback and release decoder resources. */
void splash_bg_gif_stop(void);

#ifdef __cplusplus
}
#endif
