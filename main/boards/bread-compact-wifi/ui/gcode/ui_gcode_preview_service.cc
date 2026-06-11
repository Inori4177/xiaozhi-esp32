#include "ui_gcode_preview_service.h"

#include "../cnc/ui_cnc_config.h"
#include "../pick/ui_pick_map_preview.h"
#include "ui_gcode_preview.h"

#ifndef UI_GCODE_PREVIEW_PATH_MAX
#define UI_GCODE_PREVIEW_PATH_MAX 128
#endif
#ifndef UI_GCODE_PREVIEW_MAX_BYTES
#define UI_GCODE_PREVIEW_MAX_BYTES (120 * 1024)
#endif

#include <cstring>
#include <vector>

#include <esp_heap_caps.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "gcode_preview_svc";

static char s_vfs_path[UI_GCODE_PREVIEW_PATH_MAX] = {};
static uint16_t *s_canvas_buf = nullptr;
static uint32_t s_generation = 0;
static TaskHandle_t s_task = nullptr;

static struct {
    lv_obj_t *canvas;
    lv_obj_t *name_label;
    lv_obj_t *status_label;
    bool bound;
} s_ui = {};

// RGB565 colors — match WebUI gcode_preview.js
static constexpr uint16_t kBgColor = 0xE739;      // ~#E8F4F2
static constexpr uint16_t kTravelColor = 0xBDD5;  // ~#B0BEC5
static constexpr uint16_t kEngraveColor = 0x214A; // ~#1A3A4A

struct PreviewResult {
    uint32_t generation;
    lv_obj_t *canvas;
    lv_obj_t *name_label;
    lv_obj_t *status_label;
    bool ok;
    char message[64];
    char basename[64];
    std::vector<UiGcodeSegment> segments;
};

static void apply_result_async(void *user_data)
{
    auto *result = static_cast<PreviewResult *>(user_data);
    if (result == nullptr) {
        return;
    }

    if (result->generation != s_generation) {
        delete result;
        return;
    }

    if (result->canvas != nullptr && s_canvas_buf != nullptr) {
        ui_gcode_preview_rasterize(result->ok ? result->segments : std::vector<UiGcodeSegment>{},
                                   s_canvas_buf, UI_GCODE_PREVIEW_CANVAS_PX, UI_GCODE_PREVIEW_CANVAS_PX,
                                   UI_CNC_WORK_SIZE_MM, kBgColor, kTravelColor, kEngraveColor);
        lv_canvas_set_buffer(result->canvas, s_canvas_buf, UI_GCODE_PREVIEW_CANVAS_PX,
                             UI_GCODE_PREVIEW_CANVAS_PX, LV_COLOR_FORMAT_RGB565);
        lv_obj_invalidate(result->canvas);
    }

    if (result->name_label != nullptr) {
        lv_label_set_text(result->name_label, result->basename[0] ? result->basename : "—");
    }
    if (result->status_label != nullptr) {
        lv_label_set_text(result->status_label, result->message);
    }

    delete result;
    s_task = nullptr;
}

static void preview_task(void *arg)
{
    auto *params = static_cast<PreviewResult *>(arg);
    if (params == nullptr) {
        s_task = nullptr;
        vTaskDelete(nullptr);
        return;
    }

    const uint32_t gen = params->generation;
    char path_copy[UI_GCODE_PREVIEW_PATH_MAX];
    strncpy(path_copy, s_vfs_path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    if (path_copy[0] == '\0') {
        params->ok = false;
        snprintf(params->message, sizeof(params->message), "未选择文件");
    } else {
        params->ok =
            ui_gcode_preview_parse_file(path_copy, UI_GCODE_PREVIEW_MAX_BYTES, params->segments);
        if (!params->ok) {
            snprintf(params->message, sizeof(params->message), "无法解析 G-code");
        } else {
            snprintf(params->message, sizeof(params->message), "%u 段刀路",
                     static_cast<unsigned>(params->segments.size()));
        }
        const char *base = strrchr(path_copy, '/');
        base = base != nullptr ? base + 1 : path_copy;
        strncpy(params->basename, base, sizeof(params->basename) - 1);
        params->basename[sizeof(params->basename) - 1] = '\0';
    }

    if (gen != s_generation) {
        delete params;
        s_task = nullptr;
        vTaskDelete(nullptr);
        return;
    }

    lv_async_call(apply_result_async, params);
    vTaskDelete(nullptr);
}

void ui_gcode_preview_init(void)
{
    if (s_canvas_buf != nullptr) {
        return;
    }
    const size_t bytes =
        static_cast<size_t>(UI_GCODE_PREVIEW_CANVAS_PX) * static_cast<size_t>(UI_GCODE_PREVIEW_CANVAS_PX) *
        sizeof(uint16_t);
    s_canvas_buf =
        static_cast<uint16_t *>(heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (s_canvas_buf == nullptr) {
        s_canvas_buf =
            static_cast<uint16_t *>(heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    }
    if (s_canvas_buf == nullptr) {
        ESP_LOGE(TAG, "canvas buffer alloc failed");
        return;
    }
    ui_gcode_preview_rasterize({}, s_canvas_buf, UI_GCODE_PREVIEW_CANVAS_PX, UI_GCODE_PREVIEW_CANVAS_PX,
                               UI_CNC_WORK_SIZE_MM, kBgColor, kTravelColor, kEngraveColor);
}

void ui_gcode_preview_bind_canvas(lv_obj_t *canvas)
{
    ui_gcode_preview_init();
    if (canvas == nullptr || s_canvas_buf == nullptr) {
        return;
    }
    lv_canvas_set_buffer(canvas, s_canvas_buf, UI_GCODE_PREVIEW_CANVAS_PX, UI_GCODE_PREVIEW_CANVAS_PX,
                         LV_COLOR_FORMAT_RGB565);
}

void ui_gcode_preview_bind_ui(lv_obj_t *canvas, lv_obj_t *name_label, lv_obj_t *status_label)
{
    s_ui.canvas = canvas;
    s_ui.name_label = name_label;
    s_ui.status_label = status_label;
    s_ui.bound = canvas != nullptr;
}

void ui_gcode_preview_unbind_ui(void)
{
    s_ui.bound = false;
    s_ui.canvas = nullptr;
    s_ui.name_label = nullptr;
    s_ui.status_label = nullptr;
}

void ui_gcode_preview_set_path(const char *vfs_path)
{
    if (vfs_path == nullptr) {
        s_vfs_path[0] = '\0';
        return;
    }
    strncpy(s_vfs_path, vfs_path, sizeof(s_vfs_path) - 1);
    s_vfs_path[sizeof(s_vfs_path) - 1] = '\0';
    ESP_LOGI(TAG, "preview path: %s", s_vfs_path);
    if (s_vfs_path[0] != '\0') {
        ui_pick_map_preview_load_path(s_vfs_path);
    }
}

const char *ui_gcode_preview_get_path(void)
{
    return s_vfs_path;
}

void ui_gcode_preview_cancel(void)
{
    ++s_generation;
}

void ui_gcode_preview_refresh(lv_obj_t *canvas, lv_obj_t *name_label, lv_obj_t *status_label)
{
    ui_gcode_preview_init();
    ui_gcode_preview_cancel();

    if (status_label != nullptr) {
        lv_label_set_text(status_label, "加载中…");
    }

    auto *params = new PreviewResult{};
    params->generation = s_generation;
    params->canvas = canvas;
    params->name_label = name_label;
    params->status_label = status_label;
    params->ok = false;
    params->message[0] = '\0';
    params->basename[0] = '\0';

    if (xTaskCreatePinnedToCore(preview_task, "gcode_preview", 4096, params, 4, &s_task, 0) != pdPASS) {
        ESP_LOGE(TAG, "preview task create failed");
        lv_label_set_text(status_label, "预览任务失败");
        delete params;
        s_task = nullptr;
    }
}
