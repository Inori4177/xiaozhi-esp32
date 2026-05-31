#include "webui_ws.h"
#include "webui_command.h"
#include "webui_config.h"
#include "webui_cnc_bridge.h"
#include "webui_log.h"

#include <cstdio>
#include <cstring>

#include <esp_log.h>

static const char *TAG = "webui_ws";
static httpd_handle_t s_server = nullptr;

void webui_ws_set_server(httpd_handle_t server)
{
    s_server = server;
}

void webui_ws_broadcast(const char *text)
{
    if (s_server == nullptr || text == nullptr) {
        return;
    }
    httpd_ws_frame_t frame = {};
    frame.type = HTTPD_WS_TYPE_TEXT;
    frame.payload = reinterpret_cast<uint8_t *>(const_cast<char *>(text));
    frame.len = strlen(text);
    httpd_handle_t hd = s_server;
    size_t clients = WEBUI_WS_CLIENT_MAX;
    int fds[WEBUI_WS_CLIENT_MAX] = {};
    if (httpd_get_client_list(hd, &clients, fds) != ESP_OK) {
        return;
    }
    for (size_t i = 0; i < clients; ++i) {
        if (httpd_ws_get_fd_info(hd, fds[i]) == HTTPD_WS_CLIENT_WEBSOCKET) {
            httpd_ws_send_frame_async(hd, fds[i], &frame);
        }
    }
}

static esp_err_t ws_receive_text(httpd_req_t *req, char *buf, size_t buf_size, size_t *out_len)
{
    httpd_ws_frame_t frame = {};
    frame.type = HTTPD_WS_TYPE_TEXT;
    esp_err_t ret = httpd_ws_recv_frame(req, &frame, 0);
    if (ret != ESP_OK) {
        return ret;
    }
    if (frame.len >= buf_size) {
        return ESP_ERR_INVALID_SIZE;
    }
    frame.payload = reinterpret_cast<uint8_t *>(buf);
    ret = httpd_ws_recv_frame(req, &frame, frame.len);
    if (ret == ESP_OK && out_len) {
        *out_len = frame.len;
    }
    return ret;
}

esp_err_t webui_ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        webui_log_append("WS connected\n");
        return ESP_OK;
    }
    char buf[WEBUI_CMD_LINE_MAX] = {};
    size_t len = 0;
    const esp_err_t ret = ws_receive_text(req, buf, sizeof(buf) - 1, &len);
    if (ret != ESP_OK) {
        return ret;
    }
    buf[len] = '\0';
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
        buf[--len] = '\0';
    }
    char resp[256] = {};
    if (strncmp(buf, "$J=", 3) == 0) {
        char axis[8] = {};
        float feed = 3000.0f;
        sscanf(buf + 3, "%7[^ ] F%f", axis, &feed);
        if (webui_cnc_jog(axis, feed)) {
            snprintf(resp, sizeof(resp), "ok\n");
        } else {
            snprintf(resp, sizeof(resp), "error: CNC worker not ready\n");
        }
        webui_log_append(resp);
        webui_ws_broadcast(resp);
    } else {
        (void)webui_command_dispatch_ws(buf, resp, sizeof(resp));
    }
    httpd_ws_frame_t reply = {};
    reply.type = HTTPD_WS_TYPE_TEXT;
    if (resp[0] == '\0') {
        snprintf(resp, sizeof(resp), "ok\n");
    }
    reply.payload = reinterpret_cast<uint8_t *>(resp);
    reply.len = strlen(resp);
    return httpd_ws_send_frame(req, &reply);
}
