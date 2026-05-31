#include "webui_http.h"

#include <cstring>

#include <esp_http_server.h>

// EMBED_TXTFILES symbols use the file basename (see IDF target_add_binary_data).
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t app_js_start[] asm("_binary_app_js_start");
extern const uint8_t app_js_end[] asm("_binary_app_js_end");
extern const uint8_t style_css_start[] asm("_binary_style_css_start");
extern const uint8_t style_css_end[] asm("_binary_style_css_end");
extern const uint8_t gcodegen_js_start[] asm("_binary_gcodegen_js_start");
extern const uint8_t gcodegen_js_end[] asm("_binary_gcodegen_js_end");

static esp_err_t send_blob(httpd_req_t *req, const char *content_type, const uint8_t *start,
                           const uint8_t *end, bool text_embed)
{
    size_t len = static_cast<size_t>(end - start);
    // EMBED_TXTFILES appends a null byte; browsers choke on it in .js/.html.
    if (text_embed && len > 0) {
        --len;
    }
    httpd_resp_set_type(req, content_type);
    return httpd_resp_send(req, reinterpret_cast<const char *>(start), len);
}

esp_err_t webui_static_handler(httpd_req_t *req)
{
    const char *uri = req->uri;
    if (strcmp(uri, "/") == 0 || strcmp(uri, "/index.html") == 0) {
        return send_blob(req, "text/html", index_html_start, index_html_end, true);
    }
    if (strcmp(uri, "/app.js") == 0) {
        return send_blob(req, "application/javascript", app_js_start, app_js_end, true);
    }
    if (strcmp(uri, "/style.css") == 0) {
        return send_blob(req, "text/css", style_css_start, style_css_end, true);
    }
    if (strcmp(uri, "/gcodegen.js") == 0) {
        return send_blob(req, "application/javascript", gcodegen_js_start, gcodegen_js_end, true);
    }
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "not found");
    return ESP_FAIL;
}
