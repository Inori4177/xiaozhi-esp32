#include "laser_controller.h"

#include "laser_gcode.h"
#include "laser_ui_state.h"
#include "ui/laser_ui_events.h"

#include <esp_log.h>

static const char *TAG = "laser_ctrl";

/** GRBL real-time commands for run / pause. */
static void send_run(void)
{
    laser_gcode_send("~");
}

static void send_pause(void)
{
    laser_gcode_send("!");
}

static void send_jog_axis(char axis, bool positive)
{
    float step = laser_ui_state_get_jog_step_mm();
    float delta = positive ? step : -step;
    laser_gcode_sendf("G91 G0 %c%.3f F3000", axis, static_cast<double>(delta));
    laser_gcode_send("G90");
}

static void send_home(void)
{
    laser_gcode_send("G28");
}

static void send_settings_apply(void)
{
    laser_ui_settings_t s = laser_ui_state_get_settings();

    static const char *k_materials[] = {"Wood", "Acrylic", "Leather", "Custom"};
    int mat = s.material_index;
    if (mat < 0 || mat >= 4) {
        mat = 0;
    }
    laser_gcode_sendf(";MATERIAL=%s", k_materials[mat]);

    int laser_s = s.laser_power_pct * 10;
    if (laser_s > 1000) {
        laser_s = 1000;
    }
    laser_gcode_sendf("M3 S%d", laser_s);

    int feed = LASER_GCODE_FEED_BASE_MM_MIN * s.speed_pct / 100;
    if (feed < 1) {
        feed = 1;
    }
    laser_gcode_sendf("G1 F%d", feed);
}

static void on_ui_event(laser_ui_event_id_t id, void *user_data)
{
    (void)user_data;

    switch (id) {
    case LASER_EVT_RUN:
        ESP_LOGI(TAG, "run");
        send_run();
        break;
    case LASER_EVT_PAUSE:
        ESP_LOGI(TAG, "pause");
        send_pause();
        break;
    case LASER_EVT_JOG_X_PLUS:
        send_jog_axis('X', true);
        break;
    case LASER_EVT_JOG_X_MINUS:
        send_jog_axis('X', false);
        break;
    case LASER_EVT_JOG_Y_PLUS:
        send_jog_axis('Y', true);
        break;
    case LASER_EVT_JOG_Y_MINUS:
        send_jog_axis('Y', false);
        break;
    case LASER_EVT_JOG_HOME:
        send_home();
        break;
    case LASER_EVT_STEP_CHANGED:
        ESP_LOGD(TAG, "step=%.3f mm", static_cast<double>(laser_ui_state_get_jog_step_mm()));
        break;
    case LASER_EVT_SETTINGS_APPLY:
        ESP_LOGI(TAG, "settings apply");
        send_settings_apply();
        break;
    case LASER_EVT_MATERIAL_CHANGED:
    case LASER_EVT_POWER_CHANGED:
    case LASER_EVT_SPEED_CHANGED:
        ESP_LOGD(TAG, "settings preview (apply to send)");
        break;
    default:
        break;
    }
}

void laser_controller_init(void)
{
    laser_gcode_init();
    laser_ui_state_init();
    laser_ui_events_register(on_ui_event, nullptr);
    ESP_LOGI(TAG, "event handler registered");
}
