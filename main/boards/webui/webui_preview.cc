#include "webui_preview.h"
#include "webui_config.h"
#include "webui_vfs.h"

#include <cstring>

#include <esp_http_server.h>
#include <esp_log.h>

#if CONFIG_MSP3525_LASER_UI
#include "ui/gcode/ui_gcode_preview_service.h"
#endif

static const char *TAG = "webui_preview";

static bool parse_json_string(const char *body, const char *key, char *out, size_t out_len)
{
    if (body == nullptr || key == nullptr || out == nullptr || out_len == 0) {
        return false;
    }
    char pattern[32];
    snprintf(pattern, sizeof(pattern), "\"%s\":\"", key);
    const char *p = strstr(body, pattern);
    if (p == nullptr) {
        return false;
    }
    p += strlen(pattern);
    const char *end = strchr(p, '"');
    if (end == nullptr) {
        return false;
    }
    size_t len = static_cast<size_t>(end - p);
    if (len >= out_len) {
        len = out_len - 1;
    }
    memcpy(out, p, len);
    out[len] = '\0';
    return true;
}

extern "C" esp_err_t webui_preview_handler(httpd_req_t *req)
{
    if (req->method != HTTP_POST) {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "POST only");
        return ESP_FAIL;
    }

    if (req->content_len <= 0 || req->content_len > 256) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad body");
        return ESP_FAIL;
    }

    char body[256] = {};
    const int received = httpd_req_recv(req, body, static_cast<size_t>(req->content_len));
    if (received <= 0) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "recv failed");
        return ESP_FAIL;
    }
    body[received] = '\0';

    char web_path[WEBUI_FILE_PATH_MAX] = {};
    if (!parse_json_string(body, "path", web_path, sizeof(web_path))) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing path");
        return ESP_FAIL;
    }

    char vfs_path[WEBUI_FILE_PATH_MAX] = {};
    if (!webui_vfs_resolve_path(web_path, vfs_path, sizeof(vfs_path))) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad path");
        return ESP_FAIL;
    }

#if CONFIG_MSP3525_LASER_UI
    ui_gcode_preview_set_path(vfs_path);
    ESP_LOGI(TAG, "sync preview: %s", vfs_path);
#else
    (void)vfs_path;
    ESP_LOGW(TAG, "preview sync ignored (no laser UI)");
#endif

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"Ok\"}");
    return ESP_OK;
}
