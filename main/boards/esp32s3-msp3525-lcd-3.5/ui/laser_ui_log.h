#pragma once

typedef void (*laser_ui_log_refresh_cb_t)(void);

void laser_ui_log_init(void);
void laser_ui_log_append(const char *role, const char *content);
void laser_ui_log_set_refresh_cb(laser_ui_log_refresh_cb_t cb);
const char *laser_ui_log_get_text(void);
