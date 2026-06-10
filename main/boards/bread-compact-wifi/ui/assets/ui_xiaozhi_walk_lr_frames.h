#pragma once

#include "laser_ui_images.h"

static const lv_image_dsc_t *const k_xiaozhi_walk_left_frames[] = {
    &left1,
    &left2,
    &left3,
    &left4,
    &left5,
    &left6,
};

static const lv_image_dsc_t *const k_xiaozhi_walk_right_frames[] = {
    &right1,
    &right2,
    &right3,
    &right4,
    &right5,
    &right6,
};

static constexpr unsigned k_xiaozhi_walk_lr_frame_count =
    sizeof(k_xiaozhi_walk_left_frames) / sizeof(k_xiaozhi_walk_left_frames[0]);
