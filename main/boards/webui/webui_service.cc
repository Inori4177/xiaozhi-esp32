#include "webui_service.h"
#include "webui_http.h"
#include "webui_log.h"
#include "webui_vfs.h"

#include <esp_log.h>

#if CONFIG_MSP3525_LASER_UI
extern "C" {
#include "ui/cnc/ui_cnc_print_service.h"
}
#endif

static const char *TAG = "webui";
static bool s_running = false;

void webui_service_start(void)
{
    if (s_running) {
        return;
    }
    webui_log_init();
#if CONFIG_MSP3525_LASER_UI
    ui_cnc_print_service_init();
#endif
    if (!webui_vfs_mount()) {
        ESP_LOGW(TAG, "VFS mount failed; file upload disabled");
    }
    if (webui_http_start() == nullptr) {
        ESP_LOGE(TAG, "HTTP start failed");
        return;
    }
    s_running = true;
    ESP_LOGI(TAG, "WebUI started — open http://<device-ip>/");
}

void webui_service_stop(void)
{
    if (!s_running) {
        return;
    }
    webui_http_stop(nullptr);
    webui_vfs_unmount();
    s_running = false;
}

bool webui_service_is_running(void)
{
    return s_running;
}
