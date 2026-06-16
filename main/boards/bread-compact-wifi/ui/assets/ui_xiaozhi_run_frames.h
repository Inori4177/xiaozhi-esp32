#pragma once

#include "laser_ui_images.h"

/* Only frames with generated .c assets under ui/assets/images/ */
static const lv_image_dsc_t *const k_xiaozhi_run_frames[] = {
    &run1,
    &run2,
    &run3,
    &run4,
    &run5,
    &run6,
    &run7,
};

static constexpr unsigned k_xiaozhi_run_frame_count =
    sizeof(k_xiaozhi_run_frames) / sizeof(k_xiaozhi_run_frames[0]);
