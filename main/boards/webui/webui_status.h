#pragma once

#include <esp_http_server.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t webui_status_handler(httpd_req_t *req);

#ifdef __cplusplus
}
#endif
