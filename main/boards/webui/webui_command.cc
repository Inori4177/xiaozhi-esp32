#include "webui_command.h"
#include "webui_config.h"
#include "webui_cnc_bridge.h"
#include "webui_http.h"
#include "webui_log.h"
#include "webui_vfs.h"
#include "webui_wifi.h"

#include <cstring>

#include <esp_http_server.h>
#include <esp_log.h>

#include "webui_ws.h"

static const char *TAG = "webui_cmd";

static void emit_response(const char *text, char *resp, size_t resp_size)
{
    if (text == nullptr) {
        return;
    }
    if (resp != nullptr && resp_size > 0) {
        strncpy(resp, text, resp_size - 1);
        resp[resp_size - 1] = '\0';
    }
    webui_log_append(text);
    webui_ws_broadcast(text);
}

static bool dispatch_user_command(const char *cmd, char *resp, size_t resp_size)
{
    if (cmd == nullptr || cmd[0] == '\0') {
        emit_response("ok\n", resp, resp_size);
        return true;
    }
    char line[512] = {};
    if (strncmp(cmd, "[ESP800]", 8) == 0) {
        snprintf(line, sizeof(line),
                 "FW version:1.0# FW target:xiaozhi-cnc# FW HW:local# primary sd:%s# "
                 "authentication:no# webcommunication: Sync: %d:%s# hostname:xiaozhi-cnc# axis:2",
                 WEBUI_VFS_MOUNT, WEBUI_HTTP_PORT, "device");
        emit_response(line, resp, resp_size);
        return true;
    }
    if (webui_wifi_handle_esp_command(cmd, line, sizeof(line))) {
        emit_response(line, resp, resp_size);
        return true;
    }
    if (strncmp(cmd, "[ESP700]", 8) == 0) {
        const char *web_path = cmd + 8;
        if (!webui_vfs_is_mounted() && !webui_vfs_mount()) {
            emit_response("error: LocalFS not mounted\n", resp, resp_size);
            return false;
        }
        char vfs_path[WEBUI_FILE_PATH_MAX];
        if (!webui_vfs_resolve_path(web_path, vfs_path, sizeof(vfs_path))) {
            emit_response("error: bad path\n", resp, resp_size);
            return false;
        }
        if (!webui_cnc_run_file(vfs_path)) {
            snprintf(line, sizeof(line), "error: cannot run %s (missing file or CNC busy)\n", web_path);
            emit_response(line, resp, resp_size);
            return false;
        }
        snprintf(line, sizeof(line), "ok: running %s\n", web_path);
        emit_response(line, resp, resp_size);
        return true;
    }
    if (cmd[0] == '$') {
        snprintf(line, sizeof(line), "ok %s\n", cmd);
        emit_response(line, resp, resp_size);
        return true;
    }
    if (cmd[0] == '?') {
        float x = 0, y = 0;
        webui_cnc_get_position(&x, &y);
        snprintf(line, sizeof(line), "X:%.2f Y:%.2f\n", static_cast<double>(x), static_cast<double>(y));
        emit_response(line, resp, resp_size);
        return true;
    }
    if (!webui_cnc_submit_line(cmd)) {
        emit_response("error: CNC worker not ready (low memory)\n", resp, resp_size);
        return false;
    }
    emit_response("ok\n", resp, resp_size);
    return true;
}

bool webui_command_dispatch(const char *cmd, char *resp, size_t resp_size)
{
    if (resp != nullptr && resp_size > 0) {
        resp[0] = '\0';
    }
    return dispatch_user_command(cmd, resp, resp_size);
}

extern "C" void webui_command_dispatch_async(const char *cmd)
{
    char resp[512] = {};
    (void)webui_command_dispatch(cmd, resp, sizeof(resp));
}

static esp_err_t command_get_handler(httpd_req_t *req)
{
    char query[512] = {};
    if (httpd_req_get_url_query_len(req) + 1 >= sizeof(query)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "query too long");
        return ESP_FAIL;
    }
    httpd_req_get_url_query_str(req, query, sizeof(query));
    char cmd[384] = {};
    if (httpd_query_key_value(query, "commandText", cmd, sizeof(cmd)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing commandText");
        return ESP_FAIL;
    }
    char resp[512] = {};
    (void)webui_command_dispatch(cmd, resp, sizeof(resp));
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, resp[0] != '\0' ? resp : "ok\n");
    return ESP_OK;
}

extern "C" esp_err_t webui_command_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        return command_get_handler(req);
    }
    httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "GET only");
    return ESP_FAIL;
}
