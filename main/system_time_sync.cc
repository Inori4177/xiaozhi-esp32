#include "system_time_sync.h"

#include <esp_log.h>
#include <esp_sntp.h>
#include <sys/time.h>
#include <time.h>

#include <functional>
#include <mutex>

static const char *TAG = "TimeSync";

static std::function<void()> s_on_synced;
static std::mutex s_cb_mutex;
static bool s_started = false;

static void apply_default_timezone(void)
{
    setenv("TZ", "CST-8", 1);
    tzset();
}

static void time_sync_notification_cb(struct timeval *tv)
{
    (void)tv;
    ESP_LOGI(TAG, "SNTP time synchronized");
    std::function<void()> cb;
    {
        std::lock_guard<std::mutex> lock(s_cb_mutex);
        cb = s_on_synced;
        s_on_synced = nullptr;
    }
    if (cb) {
        cb();
    }
}

bool SystemTimeSyncIsReady()
{
    time_t now = time(nullptr);
    struct tm tm_info {};
    localtime_r(&now, &tm_info);
    return tm_info.tm_year >= 2025 - 1900;
}

void SystemTimeSyncStart(std::function<void()> on_synced)
{
    if (SystemTimeSyncIsReady()) {
        if (on_synced) {
            on_synced();
        }
        return;
    }

    {
        std::lock_guard<std::mutex> lock(s_cb_mutex);
        if (on_synced) {
            s_on_synced = std::move(on_synced);
        }
    }

    if (s_started) {
        return;
    }
    s_started = true;

    apply_default_timezone();

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "ntp.aliyun.com");
    esp_sntp_setservername(1, "cn.pool.ntp.org");
    esp_sntp_setservername(2, "pool.ntp.org");
    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();
    ESP_LOGI(TAG, "SNTP started");
}
