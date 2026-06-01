#include "webui_config.h"
#include "webui_http.h"
#include "webui_vfs.h"

#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <esp_http_server.h>
#include <esp_log.h>

static const char *TAG = "webui_files";

static std::string url_decode(const char *in)
{
    std::string out;
    if (in == nullptr) {
        return out;
    }
    for (const char *p = in; *p; ++p) {
        if (*p == '%' && p[1] && p[2]) {
            char hex[3] = {p[1], p[2], 0};
            out.push_back(static_cast<char>(strtol(hex, nullptr, 16)));
            p += 2;
        } else if (*p == '+') {
            out.push_back(' ');
        } else {
            out.push_back(*p);
        }
    }
    return out;
}

/** Decode URL query value and normalize to web path (/ or /name). */
static std::string normalize_web_path(const char *encoded)
{
    if (encoded == nullptr || encoded[0] == '\0') {
        return "/";
    }
    std::string path = url_decode(encoded);
    while (!path.empty() && path[0] == '/') {
        path.erase(0, 1);
    }
    if (path.empty()) {
        return "/";
    }
    return "/" + path;
}

static esp_err_t files_list_json(const char *web_path, httpd_req_t *req)
{
    char vfs_path[WEBUI_FILE_PATH_MAX];
    if (!webui_vfs_resolve_path(web_path, vfs_path, sizeof(vfs_path))) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad path");
        return ESP_FAIL;
    }
    DIR *dir = opendir(vfs_path);
    if (dir == nullptr) {
        ESP_LOGE(TAG, "opendir failed: %s", vfs_path);
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req,
                            "{\"status\":\"Error\",\"msg\":\"opendir failed; check LocalFS mount\"}");
        return ESP_FAIL;
    }
    std::string json = "{\"status\":\"Ok\",\"path\":\"";
    json += web_path ? web_path : "/";
    json += "\",\"files\":[";
    bool first = true;
    struct dirent *ent;
    while ((ent = readdir(dir)) != nullptr) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }
        const std::string full = std::string(vfs_path) + "/" + ent->d_name;
        struct stat st = {};
        stat(full.c_str(), &st);
        if (!first) {
            json += ',';
        }
        first = false;
        json += "{\"name\":\"";
        json += ent->d_name;
        json += "\",\"size\":";
        if (S_ISDIR(st.st_mode)) {
            json += "-1";
        } else {
            json += std::to_string(static_cast<long long>(st.st_size));
        }
        json += "}";
    }
    closedir(dir);
    json += "],\"total\":\"1M\",\"used\":\"0\",\"occupation\":\"0\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json.c_str(), json.size());
    return ESP_OK;
}

static esp_err_t files_delete(const char *web_path, const char *filename, httpd_req_t *req)
{
    char vfs_path[WEBUI_FILE_PATH_MAX];
    std::string rel = web_path ? web_path : "/";
    if (!filename) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "no filename");
        return ESP_FAIL;
    }
    if (!rel.empty() && rel.back() != '/') {
        rel += '/';
    }
    rel += filename;
    if (!webui_vfs_resolve_path(rel.c_str(), vfs_path, sizeof(vfs_path))) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad path");
        return ESP_FAIL;
    }
    if (remove(vfs_path) != 0) {
        ESP_LOGE(TAG, "remove failed: %s errno=%d", vfs_path, errno);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "delete failed");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "deleted %s", vfs_path);
    return files_list_json(web_path, req);
}

static bool request_is_multipart(httpd_req_t *req)
{
    const size_t hdr_len = httpd_req_get_hdr_value_len(req, "Content-Type");
    if (hdr_len == 0 || hdr_len >= 64) {
        return false;
    }
    char content_type[64] = {};
    if (httpd_req_get_hdr_value_str(req, "Content-Type", content_type, sizeof(content_type)) != ESP_OK) {
        return false;
    }
    return strstr(content_type, "multipart/form-data") != nullptr;
}

static bool path_ends_gcode(const char *path)
{
    if (path == nullptr) {
        return false;
    }
    const size_t n = strlen(path);
    if (n >= 6 && strcasecmp(path + n - 6, ".gcode") == 0) {
        return true;
    }
    if (n >= 4 && strcasecmp(path + n - 4, ".gco") == 0) {
        return true;
    }
    if (n >= 3 && strcasecmp(path + n - 3, ".nc") == 0) {
        return true;
    }
    return false;
}

static esp_err_t files_upload_raw(httpd_req_t *req, const char *web_path)
{
    char vfs_path[WEBUI_FILE_PATH_MAX];
    if (!webui_vfs_resolve_path(web_path, vfs_path, sizeof(vfs_path))) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad path");
        return ESP_FAIL;
    }
    constexpr size_t k_upload_max = 512 * 1024;
    if (req->content_len > k_upload_max) {
        httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE, "file too large (max 512KB)");
        return ESP_FAIL;
    }
    if (path_ends_gcode(vfs_path) && req->content_len > WEBUI_GCODEGEN_MAX_BYTES) {
        httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE, "gcode too large (max 120KB)");
        return ESP_FAIL;
    }
    remove(vfs_path);
    FILE *f = fopen(vfs_path, "wb");
    if (f == nullptr) {
        ESP_LOGE(TAG, "fopen(%s) failed errno=%d (%s)", vfs_path, errno, strerror(errno));
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_set_type(req, "application/json");
        const char *hint = (errno == EINVAL)
                               ? "invalid FAT file name (enable FATFS LFN or use 8.3 name like engrave.nc)"
                               : "open failed (low memory or full disk)";
        char err_json[256];
        snprintf(err_json, sizeof(err_json), "{\"status\":\"Error\",\"msg\":\"%s\"}", hint);
        httpd_resp_sendstr(req, err_json);
        return ESP_FAIL;
    }
    char buf[1024];
    size_t received = 0;
    while (received < req->content_len) {
        size_t chunk = req->content_len - received;
        if (chunk > sizeof(buf)) {
            chunk = sizeof(buf);
        }
        const int r = httpd_req_recv(req, buf, chunk);
        if (r <= 0) {
            fclose(f);
            remove(vfs_path);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "recv failed");
            return ESP_FAIL;
        }
        if (fwrite(buf, 1, static_cast<size_t>(r), f) != static_cast<size_t>(r)) {
            fclose(f);
            remove(vfs_path);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "write failed");
            return ESP_FAIL;
        }
        received += static_cast<size_t>(r);
    }
    fclose(f);
    ESP_LOGI(TAG, "uploaded %s (%u bytes)", vfs_path, static_cast<unsigned>(received));
    return files_list_json("/", req);
}

static esp_err_t files_upload_multipart(httpd_req_t *req)
{
    const size_t buf_len = req->content_len + 1;
    if (buf_len > 256 * 1024) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "too large");
        return ESP_FAIL;
    }
    std::vector<char> body(buf_len);
    size_t received = 0;
    while (received < req->content_len) {
        const int r = httpd_req_recv(req, body.data() + received, req->content_len - received);
        if (r <= 0) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "recv failed");
            return ESP_FAIL;
        }
        received += static_cast<size_t>(r);
    }
    body[received] = '\0';

    std::string dest_name = "upload.gcode";
    const char *path_key = strstr(body.data(), "name=\"path\"");
    if (path_key) {
        const char *start = strstr(path_key, "\r\n\r\n");
        if (start) {
            start += 4;
            const char *end = strstr(start, "\r\n");
            if (end) {
                std::string p(start, end - start);
                const size_t slash = p.find_last_of('/');
                dest_name = (slash != std::string::npos) ? p.substr(slash + 1) : p;
            }
        }
    }
    const char *file_key = strstr(body.data(), "name=\"myfiles[]\"");
    if (!file_key) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "no file");
        return ESP_FAIL;
    }
    const char *hdr_end = strstr(file_key, "\r\n\r\n");
    if (!hdr_end) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad multipart");
        return ESP_FAIL;
    }
    const char *data_start = hdr_end + 4;
    const char *data_end = strstr(data_start, "\r\n--");
    if (!data_end) {
        data_end = body.data() + received;
    }
    char vfs_path[WEBUI_FILE_PATH_MAX];
    std::string rel = "/";
    rel += dest_name;
    if (!webui_vfs_resolve_path(rel.c_str(), vfs_path, sizeof(vfs_path))) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad dest");
        return ESP_FAIL;
    }
    remove(vfs_path);
    FILE *f = fopen(vfs_path, "wb");
    if (f == nullptr) {
        ESP_LOGE(TAG, "fopen(%s) failed errno=%d (%s)", vfs_path, errno, strerror(errno));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "open failed");
        return ESP_FAIL;
    }
    fwrite(data_start, 1, static_cast<size_t>(data_end - data_start), f);
    fclose(f);
    ESP_LOGI(TAG, "uploaded %s (%d bytes)", vfs_path, static_cast<int>(data_end - data_start));
    return files_list_json("/", req);
}

extern "C" esp_err_t webui_files_handler(httpd_req_t *req)
{
    if (!webui_vfs_is_mounted() && !webui_vfs_mount()) {
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req,
                            "{\"status\":\"Error\",\"msg\":\"LocalFS not mounted: partition "
                            "'storage' missing. Reflash with partitions/v2/16m_laser_ui_webui.csv "
                            "after erase-flash.\"}");
        return ESP_FAIL;
    }

    char query[256] = {};
    if (httpd_req_get_url_query_len(req) > 0 &&
        httpd_req_get_url_query_len(req) < sizeof(query)) {
        httpd_req_get_url_query_str(req, query, sizeof(query));
    }

    char action[32] = {};
    char filename[128] = {};
    char path_raw[128] = {};
    httpd_query_key_value(query, "action", action, sizeof(action));
    httpd_query_key_value(query, "filename", filename, sizeof(filename));
    httpd_query_key_value(query, "path", path_raw, sizeof(path_raw));
    const std::string web_path = normalize_web_path(path_raw);

    if (req->method == HTTP_POST && (action[0] == '\0' || strcmp(action, "upload") == 0)) {
        if (!path_raw[0] && !request_is_multipart(req)) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing path");
            return ESP_FAIL;
        }
        if (!request_is_multipart(req)) {
            return files_upload_raw(req, web_path.c_str());
        }
        return files_upload_multipart(req);
    }
    if (strcmp(action, "list") == 0 || action[0] == '\0') {
        return files_list_json(web_path.c_str(), req);
    }
    if (strcmp(action, "delete") == 0) {
        return files_delete(web_path.c_str(), url_decode(filename).c_str(), req);
    }
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "unknown action");
    return ESP_FAIL;
}
