#pragma once

typedef enum {
    LASER_EVT_NAV_SWITCH,
    LASER_EVT_JOG_X_PLUS,
    LASER_EVT_JOG_X_MINUS,
    LASER_EVT_JOG_Y_PLUS,
    LASER_EVT_JOG_Y_MINUS,
    LASER_EVT_JOG_HOME,
    LASER_EVT_RUN,
    LASER_EVT_PAUSE,
    LASER_EVT_STEP_CHANGED,
    LASER_EVT_MATERIAL_CHANGED,
    LASER_EVT_POWER_CHANGED,
    LASER_EVT_SPEED_CHANGED,
    LASER_EVT_SETTINGS_APPLY,
    LASER_EVT_PICK_CONFIRM,
    LASER_EVT_PICK_RESET,
} laser_ui_event_id_t;

typedef void (*laser_ui_event_handler_t)(laser_ui_event_id_t id, void *user_data);

void laser_ui_events_init(void);
void laser_ui_events_register(laser_ui_event_handler_t handler, void *user_data);
void laser_ui_events_emit(laser_ui_event_id_t id);

/** Debug: human-readable event name for logging. */
const char *laser_ui_event_name(laser_ui_event_id_t id);
