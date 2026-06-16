#pragma once

#include "laser_ui_images.h"

static const lv_image_dsc_t *const k_xiaozhi_speak_loop_frames[] = {
    &speak_1open,
    &speak_2close,
    &speak_3open,
    &speak_2close,
};

static constexpr unsigned k_xiaozhi_speak_loop_frame_count =
    sizeof(k_xiaozhi_speak_loop_frames) / sizeof(k_xiaozhi_speak_loop_frames[0]);
