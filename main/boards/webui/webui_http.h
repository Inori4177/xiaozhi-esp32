#pragma once

#include <esp_http_server.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t webui_files_handler(httpd_req_t *req);
esp_err_t webui_command_handler(httpd_req_t *req);
esp_err_t webui_static_handler(httpd_req_t *req);
esp_err_t webui_status_handler(httpd_req_t *req);
esp_err_t webui_settings_handler(httpd_req_t *req);
esp_err_t webui_pick_handler(httpd_req_t *req);

httpd_handle_t webui_http_start(void);
void webui_http_stop(httpd_handle_t server);

#ifdef __cplusplus
}
#endif
