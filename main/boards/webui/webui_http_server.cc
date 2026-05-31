#include "webui_http.h"
#include "webui_config.h"
#include "webui_ws.h"

#include <esp_log.h>

static const char *TAG = "webui_http";
static httpd_handle_t s_server = nullptr;

httpd_handle_t webui_http_start(void)
{
    if (s_server != nullptr) {
        return s_server;
    }
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.server_port = WEBUI_HTTP_PORT;
    cfg.uri_match_fn = httpd_uri_match_wildcard;
    cfg.max_uri_handlers = 18;
    cfg.stack_size = 8192;
    cfg.lru_purge_enable = true;

    if (httpd_start(&s_server, &cfg) != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed");
        s_server = nullptr;
        return nullptr;
    }

    static const httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = webui_static_handler,
        .user_ctx = nullptr,
    };
    static const httpd_uri_t index_uri = {
        .uri = "/index.html",
        .method = HTTP_GET,
        .handler = webui_static_handler,
        .user_ctx = nullptr,
    };
    static const httpd_uri_t app_js_uri = {
        .uri = "/app.js",
        .method = HTTP_GET,
        .handler = webui_static_handler,
        .user_ctx = nullptr,
    };
    static const httpd_uri_t style_uri = {
        .uri = "/style.css",
        .method = HTTP_GET,
        .handler = webui_static_handler,
        .user_ctx = nullptr,
    };
    static const httpd_uri_t gcodegen_uri = {
        .uri = "/gcodegen.js",
        .method = HTTP_GET,
        .handler = webui_static_handler,
        .user_ctx = nullptr,
    };
    static const httpd_uri_t cmd_uri = {
        .uri = WEBUI_CMD_PATH,
        .method = HTTP_GET,
        .handler = webui_command_handler,
        .user_ctx = nullptr,
    };
    static const httpd_uri_t files_uri = {
        .uri = WEBUI_FILES_PATH,
        .method = HTTP_GET,
        .handler = webui_files_handler,
        .user_ctx = nullptr,
    };
    static const httpd_uri_t files_post_uri = {
        .uri = WEBUI_FILES_PATH,
        .method = HTTP_POST,
        .handler = webui_files_handler,
        .user_ctx = nullptr,
    };
    static const httpd_uri_t ws_uri = {
        .uri = WEBUI_WS_PATH,
        .method = HTTP_GET,
        .handler = webui_ws_handler,
        .user_ctx = nullptr,
        .is_websocket = true,
    };
    static const httpd_uri_t status_uri = {
        .uri = WEBUI_STATUS_PATH,
        .method = HTTP_GET,
        .handler = webui_status_handler,
        .user_ctx = nullptr,
    };
    static const httpd_uri_t settings_uri = {
        .uri = WEBUI_SETTINGS_PATH,
        .method = HTTP_POST,
        .handler = webui_settings_handler,
        .user_ctx = nullptr,
    };
    static const httpd_uri_t pick_uri = {
        .uri = WEBUI_PICK_PATH,
        .method = HTTP_POST,
        .handler = webui_pick_handler,
        .user_ctx = nullptr,
    };
    static const httpd_uri_t run_uri = {
        .uri = WEBUI_RUN_PATH,
        .method = HTTP_POST,
        .handler = webui_run_handler,
        .user_ctx = nullptr,
    };
    static const httpd_uri_t pause_uri = {
        .uri = WEBUI_PAUSE_PATH,
        .method = HTTP_POST,
        .handler = webui_pause_handler,
        .user_ctx = nullptr,
    };
    static const httpd_uri_t chat_uri = {
        .uri = WEBUI_CHAT_PATH,
        .method = HTTP_GET,
        .handler = webui_chat_handler,
        .user_ctx = nullptr,
    };

    httpd_register_uri_handler(s_server, &root_uri);
    httpd_register_uri_handler(s_server, &index_uri);
    httpd_register_uri_handler(s_server, &app_js_uri);
    httpd_register_uri_handler(s_server, &style_uri);
    httpd_register_uri_handler(s_server, &gcodegen_uri);
    httpd_register_uri_handler(s_server, &cmd_uri);
    httpd_register_uri_handler(s_server, &files_uri);
    httpd_register_uri_handler(s_server, &files_post_uri);
    httpd_register_uri_handler(s_server, &ws_uri);
    httpd_register_uri_handler(s_server, &status_uri);
    httpd_register_uri_handler(s_server, &settings_uri);
    httpd_register_uri_handler(s_server, &pick_uri);
    httpd_register_uri_handler(s_server, &run_uri);
    httpd_register_uri_handler(s_server, &pause_uri);
    httpd_register_uri_handler(s_server, &chat_uri);

    webui_ws_set_server(s_server);
    ESP_LOGI(TAG, "HTTP server on port %d", WEBUI_HTTP_PORT);
    return s_server;
}

void webui_http_stop(httpd_handle_t server)
{
    httpd_handle_t hd = server != nullptr ? server : s_server;
    if (hd != nullptr) {
        httpd_stop(hd);
    }
    s_server = nullptr;
    webui_ws_set_server(nullptr);
}
