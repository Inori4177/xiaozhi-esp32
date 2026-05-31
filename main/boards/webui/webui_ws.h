#pragma once

#include <esp_http_server.h>

#ifdef __cplusplus
extern "C" {
#endif

void webui_ws_set_server(httpd_handle_t server);
void webui_ws_broadcast(const char *text);
esp_err_t webui_ws_handler(httpd_req_t *req);

#ifdef __cplusplus
}
#endif
