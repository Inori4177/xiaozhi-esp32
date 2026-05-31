#pragma once

#include <esp_http_server.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t webui_settings_handler(httpd_req_t *req);
esp_err_t webui_pick_handler(httpd_req_t *req);
esp_err_t webui_run_handler(httpd_req_t *req);
esp_err_t webui_pause_handler(httpd_req_t *req);

#ifdef __cplusplus
}
#endif
