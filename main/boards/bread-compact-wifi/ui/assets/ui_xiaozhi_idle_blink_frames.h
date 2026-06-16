#pragma once

#include "laser_ui_images.h"

static const lv_image_dsc_t *const k_xiaozhi_idle_blink_frames[] = {
    &blink1,
    &blink2,
    &blink3,
    &blink4,
    &blink5,
    &blink6,
    &blink7,
    &blink8,
};

static constexpr unsigned k_xiaozhi_idle_blink_frame_count =
    sizeof(k_xiaozhi_idle_blink_frames) / sizeof(k_xiaozhi_idle_blink_frames[0]);
