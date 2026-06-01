#include "ui_cnc_motion_facade.h"

#include "ui_cnc_print_service.h"

#include <esp_log.h>

static const char *TAG = "ui_cnc_motion";

static float s_pos_x = 0.0f;
static float s_pos_y = 0.0f;
static bool s_moving = false;

void ui_cnc_motion_facade_init(void)
{
    ESP_LOGI(TAG, "motion facade (peer position cache)");
}

bool ui_cnc_motion_facade_is_ready(void)
{
#if CONFIG_INTERACTION_PEER_UART
    return ui_cnc_print_service_worker_ready();
#else
    return true;
#endif
}

void ui_cnc_motion_facade_get_position_mm(float *x_mm, float *y_mm)
{
#if CONFIG_INTERACTION_PEER_UART
    ui_cnc_print_service_get_position_mm(x_mm, y_mm);
#else
    if (x_mm) {
        *x_mm = s_pos_x;
    }
    if (y_mm) {
        *y_mm = s_pos_y;
    }
#endif
}

void ui_cnc_motion_facade_get_display_position_mm(float *x_mm, float *y_mm)
{
    ui_cnc_motion_facade_get_position_mm(x_mm, y_mm);
}

void ui_cnc_motion_facade_set_position_mm(float x_mm, float y_mm)
{
    s_pos_x = x_mm;
    s_pos_y = y_mm;
}

void ui_cnc_motion_facade_begin_segment_mm(float from_x, float from_y, float to_x, float to_y,
                                           float feed_mm_min)
{
    (void)from_x;
    (void)from_y;
    (void)feed_mm_min;
    s_moving = true;
    s_pos_x = to_x;
    s_pos_y = to_y;
}

void ui_cnc_motion_facade_end_segment_mm(float x_mm, float y_mm)
{
    s_pos_x = x_mm;
    s_pos_y = y_mm;
    s_moving = false;
}

void ui_cnc_motion_facade_abort_segment_mm(void)
{
    s_moving = false;
}

bool ui_cnc_motion_facade_rapid_to_mm(float x_mm, float y_mm)
{
#if CONFIG_INTERACTION_PEER_UART
    return ui_cnc_print_service_move_to_mm_async(x_mm, y_mm);
#else
    s_pos_x = x_mm;
    s_pos_y = y_mm;
    return true;
#endif
}

void ui_cnc_motion_facade_rapid_to_mm_async(float x_mm, float y_mm)
{
#if CONFIG_INTERACTION_PEER_UART
    ui_cnc_print_service_move_to_mm_async(x_mm, y_mm);
#else
    s_pos_x = x_mm;
    s_pos_y = y_mm;
#endif
}

bool ui_cnc_motion_facade_is_moving(void)
{
#if CONFIG_INTERACTION_PEER_UART
    return ui_cnc_print_service_is_busy();
#else
    return s_moving;
#endif
}
