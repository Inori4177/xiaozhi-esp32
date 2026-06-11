#include "ui_pick_map_preview.h"

#include "../gcode/ui_gcode_preview.h"
#include "../cnc/ui_cnc_coord_map.h"
#include "../cnc/ui_cnc_print_service.h"
#include "../../laser_ui_state.h"

#include <cmath>
#include <cstring>
#include <vector>

#include <esp_heap_caps.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "pick_map_prev";

#ifndef UI_GCODE_PREVIEW_MAX_BYTES
#define UI_GCODE_PREVIEW_MAX_BYTES (120 * 1024)
#endif

#ifndef UI_PICK_MAP_PREVIEW_MAX_PX
#define UI_PICK_MAP_PREVIEW_MAX_PX 280
#endif

enum class PickMapPreviewMode : uint8_t {
    Idle = 0,
    Static,
    Running,
};

static lv_obj_t *g_map_frame = nullptr;
static lv_obj_t *g_canvas = nullptr;
static uint32_t *g_canvas_buf = nullptr;
static int g_canvas_side = 0;
static ui_cnc_coord_viewport_t g_vp{};

static PickMapPreviewMode g_mode = PickMapPreviewMode::Idle;
static std::vector<UiGcodeSegment> g_segments_raw;
static std::vector<UiGcodeSegment> g_segments;
static float g_origin_ox = 0.0f;
static float g_origin_oy = 0.0f;
static bool g_origin_valid = false;
static uint8_t g_run_progress_pct = 0;
static uint32_t g_total_send_lines = 0;
static bool g_page_visible = false;
static bool g_render_dirty = false;

static uint32_t g_generation = 0;
static TaskHandle_t g_parse_task = nullptr;
static char g_cached_path[128] = {};

static constexpr uint32_t kDoneArgb = 0xFF1A1A1AU;

static void cache_path(const char *vfs_path)
{
    if (vfs_path == nullptr) {
        g_cached_path[0] = '\0';
        return;
    }
    strncpy(g_cached_path, vfs_path, sizeof(g_cached_path) - 1);
    g_cached_path[sizeof(g_cached_path) - 1] = '\0';
}

static bool is_cnc_job_active(void)
{
    ui_cnc_print_status_t st{};
    ui_cnc_print_service_get_status(&st);
    return st.state == UI_CNC_WORK_RUNNING || st.state == UI_CNC_WORK_PAUSED;
}

static void sync_origin_from_state(void)
{
    g_origin_valid = laser_ui_state_get_pick_origin(&g_origin_ox, &g_origin_oy);
    if (!g_origin_valid) {
        g_origin_ox = 0.0f;
        g_origin_oy = 0.0f;
    }
}

static void rebuild_display_segments(void)
{
    if (g_segments_raw.empty()) {
        g_segments.clear();
        return;
    }
    g_segments = g_segments_raw;
    if (g_origin_valid) {
        ui_gcode_preview_offset_segments(g_segments, g_origin_ox, g_origin_oy);
    }
}

static uint32_t power_to_pending_green_argb(int power_pct)
{
    if (power_pct < 0) {
        power_pct = 0;
    }
    if (power_pct > 100) {
        power_pct = 100;
    }
    const float t = static_cast<float>(power_pct) / 100.0f;
    const int r = static_cast<int>(0xE8 + (0x66 - 0xE8) * t);
    const int g = static_cast<int>(0xF5 + (0xBB - 0xF5) * t);
    const int b = static_cast<int>(0xE9 + (0x6A - 0xE9) * t);
    return 0xFF000000U | (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) |
           static_cast<uint32_t>(b);
}

static void update_viewport(void)
{
    if (g_map_frame == nullptr || g_canvas_side <= 0) {
        return;
    }
    ui_cnc_coord_viewport_init(&g_vp, g_canvas_side, g_canvas_side, 0);
}

static bool ensure_canvas_buffer(int side)
{
    if (side <= 0 || side > UI_PICK_MAP_PREVIEW_MAX_PX) {
        return false;
    }
    if (g_canvas_buf != nullptr && g_canvas_side == side) {
        return true;
    }

    if (g_canvas_buf != nullptr) {
        heap_caps_free(g_canvas_buf);
        g_canvas_buf = nullptr;
    }

    const size_t bytes = static_cast<size_t>(side) * static_cast<size_t>(side) * sizeof(uint32_t);
    g_canvas_buf = static_cast<uint32_t *>(heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (g_canvas_buf == nullptr) {
        g_canvas_buf = static_cast<uint32_t *>(heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    }
    if (g_canvas_buf == nullptr) {
        ESP_LOGE(TAG, "canvas alloc failed side=%d", side);
        g_canvas_side = 0;
        return false;
    }

    g_canvas_side = side;
    update_viewport();

    if (g_canvas != nullptr) {
        lv_canvas_set_buffer(g_canvas, g_canvas_buf, g_canvas_side, g_canvas_side,
                             LV_COLOR_FORMAT_ARGB8888);
    }
    return true;
}

static uint8_t current_progress_pct(void)
{
    if (g_mode == PickMapPreviewMode::Running && is_cnc_job_active()) {
        ui_cnc_print_status_t st{};
        ui_cnc_print_service_get_status(&st);
        return st.progress_pct;
    }
    return g_run_progress_pct;
}

static void rasterize_to_canvas(void)
{
    if (g_canvas == nullptr || g_canvas_buf == nullptr || g_canvas_side <= 0) {
        return;
    }
    if (g_segments.empty()) {
        return;
    }

    update_viewport();
    const laser_ui_settings_t settings = laser_ui_state_get_settings();
    const uint32_t pending = power_to_pending_green_argb(settings.laser_power_pct);

    if (g_mode == PickMapPreviewMode::Running && is_cnc_job_active()) {
        const uint8_t pct = current_progress_pct();
        ui_gcode_preview_rasterize_progress_argb(g_segments, pending, kDoneArgb, pct,
                                                 g_total_send_lines, g_canvas_buf, g_canvas_side,
                                                 g_canvas_side, &g_vp);
    } else if (g_mode == PickMapPreviewMode::Static) {
        std::vector<uint32_t> colors(g_segments.size(), pending);
        ui_gcode_preview_rasterize_colored_argb(g_segments, colors, g_canvas_buf, g_canvas_side,
                                                g_canvas_side, &g_vp);
    } else {
        return;
    }

    lv_obj_invalidate(g_canvas);
}

static void apply_visible_state(void)
{
    if (g_canvas == nullptr) {
        return;
    }
    const bool show =
        g_page_visible && g_mode != PickMapPreviewMode::Idle && !g_segments.empty();
    if (show) {
        lv_obj_remove_flag(g_canvas, LV_OBJ_FLAG_HIDDEN);
        rasterize_to_canvas();
    } else {
        lv_obj_add_flag(g_canvas, LV_OBJ_FLAG_HIDDEN);
    }
}

static void reset_progress(void)
{
    g_run_progress_pct = 0;
}

struct ParseJob {
    uint32_t generation;
    char path[128];
    PickMapPreviewMode target_mode;
};

static void apply_parse_result_async(void *user_data)
{
    auto *job = static_cast<ParseJob *>(user_data);
    if (job == nullptr) {
        return;
    }

    if (job->generation != g_generation) {
        delete job;
        return;
    }

    std::vector<UiGcodeSegment> parsed;
    uint32_t send_lines = 0;
    const bool ok = ui_gcode_preview_parse_file_for_send(job->path, UI_GCODE_PREVIEW_MAX_BYTES,
                                                         parsed, &send_lines);
    if (!ok) {
        ESP_LOGW(TAG, "parse failed: %s", job->path);
        g_mode = PickMapPreviewMode::Idle;
        g_segments_raw.clear();
        g_segments.clear();
        apply_visible_state();
        delete job;
        return;
    }

    g_segments_raw = std::move(parsed);
    g_total_send_lines = send_lines;
    sync_origin_from_state();
    rebuild_display_segments();
    g_mode = job->target_mode;
    reset_progress();
    g_render_dirty = true;
    apply_visible_state();
    ESP_LOGI(TAG, "loaded %u segments %u send-lines mode=%u",
             static_cast<unsigned>(g_segments.size()), static_cast<unsigned>(g_total_send_lines),
             static_cast<unsigned>(g_mode));
    delete job;
}

static void parse_task(void *arg)
{
    auto *job = static_cast<ParseJob *>(arg);
    if (job == nullptr) {
        g_parse_task = nullptr;
        vTaskDelete(nullptr);
        return;
    }
    lv_async_call(apply_parse_result_async, job);
    g_parse_task = nullptr;
    vTaskDelete(nullptr);
}

static void start_parse_async(const char *vfs_path, PickMapPreviewMode mode)
{
    if (vfs_path == nullptr || vfs_path[0] == '\0') {
        return;
    }

    ++g_generation;
    if (g_parse_task != nullptr) {
        /* New generation invalidates in-flight work. */
    }

    auto *job = new ParseJob{};
    job->generation = g_generation;
    job->target_mode = mode;
    strncpy(job->path, vfs_path, sizeof(job->path) - 1);
    job->path[sizeof(job->path) - 1] = '\0';

    if (xTaskCreatePinnedToCore(parse_task, "pick_map_parse", 4096, job, 4, &g_parse_task, 0) !=
        pdPASS) {
        ESP_LOGE(TAG, "parse task create failed");
        delete job;
        g_parse_task = nullptr;
    }
}

static void rerender_async_cb(void *user_data)
{
    (void)user_data;
    if (!g_render_dirty) {
        return;
    }
    g_render_dirty = false;
    if (g_mode != PickMapPreviewMode::Idle && !g_segments.empty()) {
        rasterize_to_canvas();
    }
}

static void schedule_rerender(void)
{
    g_render_dirty = true;
    lv_async_call(rerender_async_cb, nullptr);
}

void ui_pick_map_preview_init(void)
{
    g_mode = PickMapPreviewMode::Idle;
}

lv_obj_t *ui_pick_map_preview_create_overlay(lv_obj_t *map_frame)
{
    g_map_frame = map_frame;
    g_canvas = lv_canvas_create(map_frame);
    lv_obj_set_size(g_canvas, 1, 1);
    lv_obj_align(g_canvas, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_opa(g_canvas, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_canvas, 0, LV_PART_MAIN);
    lv_obj_remove_flag(g_canvas, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(g_canvas, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(g_canvas, LV_OBJ_FLAG_HIDDEN);
    return g_canvas;
}

void ui_pick_map_preview_on_map_resized(int side_px)
{
    if (g_canvas == nullptr || side_px <= 0) {
        return;
    }
    if (!ensure_canvas_buffer(side_px)) {
        return;
    }
    lv_obj_set_size(g_canvas, side_px, side_px);
    lv_obj_align(g_canvas, LV_ALIGN_TOP_LEFT, 0, 0);
    if (g_mode != PickMapPreviewMode::Idle && !g_segments.empty()) {
        schedule_rerender();
    }
}

void ui_pick_map_preview_on_page_show(void)
{
    g_page_visible = true;
    if (is_cnc_job_active() && !g_segments.empty()) {
        g_mode = PickMapPreviewMode::Running;
    }
    apply_visible_state();
}

void ui_pick_map_preview_on_page_hide(void)
{
    g_page_visible = false;
    if (g_canvas != nullptr) {
        lv_obj_add_flag(g_canvas, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_pick_map_preview_load_path(const char *vfs_path)
{
    if (vfs_path == nullptr || vfs_path[0] == '\0') {
        return;
    }
    if (g_map_frame != nullptr && g_canvas_side <= 0) {
        const int side = lv_obj_get_width(g_map_frame);
        if (side > 0) {
            ui_pick_map_preview_on_map_resized(side);
        }
    }
    cache_path(vfs_path);
    start_parse_async(vfs_path, PickMapPreviewMode::Static);
}

void ui_pick_map_preview_start_run(const char *vfs_path)
{
    if (vfs_path == nullptr || vfs_path[0] == '\0') {
        return;
    }
    if (g_map_frame != nullptr && g_canvas_side <= 0) {
        const int side = lv_obj_get_width(g_map_frame);
        if (side > 0) {
            ui_pick_map_preview_on_map_resized(side);
        }
    }

    const bool had_same_path =
        (g_cached_path[0] != '\0' && strcmp(g_cached_path, vfs_path) == 0);
    cache_path(vfs_path);
    g_mode = PickMapPreviewMode::Running;
    reset_progress();

    if (!g_segments_raw.empty() && had_same_path) {
        sync_origin_from_state();
        rebuild_display_segments();
        apply_visible_state();
        return;
    }

    start_parse_async(vfs_path, PickMapPreviewMode::Running);
}

void ui_pick_map_preview_on_position_mm(float x_mm, float y_mm)
{
    (void)x_mm;
    (void)y_mm;
}

void ui_pick_map_preview_on_job_progress(uint8_t progress_pct)
{
    if (g_mode != PickMapPreviewMode::Running || g_segments.empty()) {
        return;
    }
    if (!is_cnc_job_active()) {
        return;
    }
    if (progress_pct == g_run_progress_pct) {
        return;
    }
    g_run_progress_pct = progress_pct;
    schedule_rerender();
}

void ui_pick_map_preview_clear(void)
{
    ++g_generation;
    g_mode = PickMapPreviewMode::Idle;
    g_segments_raw.clear();
    g_segments.clear();
    g_run_progress_pct = 0;
    g_total_send_lines = 0;
    g_cached_path[0] = '\0';
    apply_visible_state();
}

void ui_pick_map_preview_refresh_colors(void)
{
    if (g_mode == PickMapPreviewMode::Static && !g_segments.empty()) {
        schedule_rerender();
    } else if (g_mode == PickMapPreviewMode::Running && is_cnc_job_active()) {
        schedule_rerender();
    }
}

void ui_pick_map_preview_on_origin_changed(void)
{
    if (g_segments_raw.empty() || g_mode == PickMapPreviewMode::Idle) {
        return;
    }
    sync_origin_from_state();
    rebuild_display_segments();
    schedule_rerender();
    apply_visible_state();
    ESP_LOGI(TAG, "origin refresh valid=%d (%.1f, %.1f)", g_origin_valid ? 1 : 0,
             static_cast<double>(g_origin_ox), static_cast<double>(g_origin_oy));
}

