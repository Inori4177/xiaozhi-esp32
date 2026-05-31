#include "webui_wifi.h"

#include <cstdio>
#include <cstring>
#include <string>

#include <esp_log.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <wifi_manager.h>
#include <wifi_station.h>
#include <ssid_manager.h>

static const char *TAG = "webui_wifi";

static bool copy_response(const char *msg, char *out, size_t out_size)
{
    if (out == nullptr || out_size == 0) {
        return false;
    }
    snprintf(out, out_size, "%s", msg ? msg : "");
    return true;
}

static void handle_scan(char *out, size_t out_size)
{
    wifi_scan_config_t scan_cfg = {};
    scan_cfg.show_hidden = false;
    esp_wifi_scan_stop();
    if (esp_wifi_scan_start(&scan_cfg, true) != ESP_OK) {
        copy_response("error: scan failed", out, out_size);
        return;
    }
    uint16_t count = 0;
    esp_wifi_scan_get_ap_num(&count);
    if (count > 20) {
        count = 20;
    }
    wifi_ap_record_t *records =
        static_cast<wifi_ap_record_t *>(calloc(count, sizeof(wifi_ap_record_t)));
    if (records == nullptr) {
        copy_response("error: no mem", out, out_size);
        return;
    }
    esp_wifi_scan_get_ap_records(&count, records);
    size_t off = 0;
    off += snprintf(out + off, out_size - off, "AP list:\n");
    for (uint16_t i = 0; i < count && off + 64 < out_size; ++i) {
        off += snprintf(out + off, out_size - off, "%s\t%d\t%s\n", records[i].ssid,
                        records[i].rssi,
                        (records[i].authmode == WIFI_AUTH_OPEN) ? "open" : "protected");
    }
    free(records);
}

static void handle_sta_connect(const char *payload, char *out, size_t out_size)
{
    if (payload == nullptr) {
        copy_response("error: empty", out, out_size);
        return;
    }
    std::string line(payload);
    const size_t sep = line.find('|');
    if (sep == std::string::npos) {
        copy_response("error: use SSID|password", out, out_size);
        return;
    }
    const std::string ssid = line.substr(0, sep);
    const std::string password = line.substr(sep + 1);
    SsidManager::GetInstance().AddSsid(ssid, password);
    auto &wifi = WifiManager::GetInstance();
    if (!wifi.IsInitialized()) {
        WifiManagerConfig cfg = {};
        cfg.ssid_prefix = "Xiaozhi";
        wifi.Initialize(cfg);
    }
    wifi.StopStation();
    vTaskDelay(pdMS_TO_TICKS(200));
    wifi.StartStation();
    snprintf(out, out_size, "ok: connecting to %s", ssid.c_str());
    ESP_LOGI(TAG, "STA connect requested: %s", ssid.c_str());
}

bool webui_wifi_handle_esp_command(const char *cmd, char *out, size_t out_size)
{
    if (cmd == nullptr || out == nullptr) {
        return false;
    }
    if (strncmp(cmd, "[ESP410]", 8) == 0) {
        handle_scan(out, out_size);
        return true;
    }
    if (strncmp(cmd, "[ESP401]", 8) == 0 || strncmp(cmd, "[ESP420]", 8) == 0) {
        const char *payload = cmd + 8;
        handle_sta_connect(payload, out, out_size);
        return true;
    }
    return false;
}
