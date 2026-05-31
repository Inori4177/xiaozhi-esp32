#include "webui_vfs.h"
#include "webui_config.h"

#include <cstdio>
#include <cstring>

#include <esp_log.h>
#include <esp_partition.h>
#include <esp_vfs_fat.h>

static const char *TAG = "webui_vfs";
static wl_handle_t s_wl = WL_INVALID_HANDLE;
static bool s_mounted = false;

bool webui_vfs_mount(void)
{
    if (s_mounted) {
        return true;
    }
    esp_vfs_fat_mount_config_t cfg = {};
    cfg.format_if_mount_failed = true;
    cfg.max_files = 12;
    cfg.allocation_unit_size = 4096;
    cfg.disk_status_check_enable = false;
    cfg.use_one_fat = false;
    const esp_err_t err =
        esp_vfs_fat_spiflash_mount_rw_wl(WEBUI_VFS_MOUNT, WEBUI_VFS_LABEL, &cfg, &s_wl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mount failed: %s (label=%s)", esp_err_to_name(err), WEBUI_VFS_LABEL);
        if (err == ESP_ERR_NOT_FOUND) {
            esp_partition_iterator_t it =
                esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, nullptr);
            while (it != nullptr) {
                const esp_partition_t *p = esp_partition_get(it);
                ESP_LOGW(TAG, "data partition: label=%s subtype=%u size=0x%x", p->label,
                         static_cast<unsigned>(p->subtype), static_cast<unsigned>(p->size));
                it = esp_partition_next(it);
            }
            esp_partition_iterator_release(it);
            ESP_LOGW(TAG, "use partitions/v2/16m_laser_ui_webui.csv and erase-flash before reflash");
        }
        return false;
    }
    s_mounted = true;
    ESP_LOGI(TAG, "mounted at %s", WEBUI_VFS_MOUNT);
    return true;
}

void webui_vfs_unmount(void)
{
    if (!s_mounted) {
        return;
    }
    esp_vfs_fat_spiflash_unmount_rw_wl(WEBUI_VFS_MOUNT, s_wl);
    s_wl = WL_INVALID_HANDLE;
    s_mounted = false;
}

bool webui_vfs_is_mounted(void)
{
    return s_mounted;
}

bool webui_vfs_resolve_path(const char *web_path, char *out, size_t out_size)
{
    if (web_path == nullptr || out == nullptr || out_size < 2) {
        return false;
    }
    const char *rel = web_path;
    while (*rel == '/') {
        ++rel;
    }
    if (rel[0] == '\0') {
        snprintf(out, out_size, "%s", WEBUI_VFS_MOUNT);
        return true;
    }
    snprintf(out, out_size, "%s/%s", WEBUI_VFS_MOUNT, rel);
    return true;
}
