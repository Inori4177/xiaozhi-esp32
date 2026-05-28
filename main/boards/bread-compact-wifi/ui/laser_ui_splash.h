#pragma once

#include <lvgl.h>

typedef enum {
    SPLASH_STEP_DISPLAY = 0,
    SPLASH_STEP_AUDIO,
    SPLASH_STEP_MCP,
    SPLASH_STEP_NETWORK,
    SPLASH_STEP_WIFI,
    SPLASH_STEP_ASSETS,
    SPLASH_STEP_OTA,
    SPLASH_STEP_PROTOCOL,
    SPLASH_STEP_READY,
    SPLASH_STEP_COUNT,
} laser_splash_step_id_t;

typedef enum {
    SPLASH_STATE_WAIT = 0,
    SPLASH_STATE_RUNNING,
    SPLASH_STATE_OK,
    SPLASH_STATE_FAIL,
} laser_splash_state_t;

class Display;

void laser_ui_splash_start(Display *display);
void laser_ui_splash_set_step(laser_splash_step_id_t step, laser_splash_state_t state);
void laser_ui_splash_finish(Display *display);
bool laser_ui_splash_is_active(void);
