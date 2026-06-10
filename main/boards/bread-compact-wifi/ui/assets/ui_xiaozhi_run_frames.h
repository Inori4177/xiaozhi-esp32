#pragma once

#include "laser_ui_images.h"

static const lv_image_dsc_t *const k_xiaozhi_run_frames[] = {
    &matte_00001,
    &matte_00002,
    &matte_00003,
    &matte_00004,
    &matte_00005,
    &matte_00006,
    &matte_00007,
    &matte_00008,
    &matte_00009,
    &matte_00010,
    &matte_00011,
    &matte_00012,
    &matte_00013,
    &matte_00014,
    &matte_00015,
    &matte_00016,
    &matte_00017,
    &matte_00018,
};

static constexpr unsigned k_xiaozhi_run_frame_count =
    sizeof(k_xiaozhi_run_frames) / sizeof(k_xiaozhi_run_frames[0]);
