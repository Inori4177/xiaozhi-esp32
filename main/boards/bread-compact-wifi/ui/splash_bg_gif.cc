#include "splash_bg_gif.h"

#include "display/lvgl_display/gif/lvgl_gif.h"

#include <esp_log.h>
#include <memory>

static const char *TAG = "splash_bg_gif";

static std::unique_ptr<LvglGif> g_gif;
static lv_obj_t *g_img = nullptr;

extern "C" {

lv_obj_t *splash_bg_gif_start(lv_obj_t *parent)
{
    splash_bg_gif_stop();
    if (parent == nullptr || splash_bg_gif_size == 0) {
        return nullptr;
    }

    lv_img_dsc_t raw = {};
    raw.header.magic = LV_IMAGE_HEADER_MAGIC;
    raw.header.cf = LV_COLOR_FORMAT_RAW_ALPHA;
    raw.data = splash_bg_gif;
    raw.data_size = splash_bg_gif_size;

    g_gif = std::make_unique<LvglGif>(&raw);
    if (!g_gif->IsLoaded()) {
        ESP_LOGW(TAG, "Failed to load splash GIF");
        g_gif.reset();
        return nullptr;
    }

    g_img = lv_image_create(parent);
    lv_image_set_src(g_img, g_gif->image_dsc());
    lv_image_set_scale(g_img, 128 * LV_HOR_RES / g_gif->width());
    lv_obj_align(g_img, LV_ALIGN_CENTER, 0, 0);
    lv_obj_remove_flag(g_img, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(g_img, LV_OBJ_FLAG_SCROLLABLE);

    g_gif->SetFrameCallback([]() {
        if (g_img != nullptr && g_gif != nullptr) {
            lv_image_set_src(g_img, g_gif->image_dsc());
        }
    });
    g_gif->Start();

    lv_obj_move_background(g_img);
    ESP_LOGI(TAG, "Splash GIF started (%ux%u)", g_gif->width(), g_gif->height());
    return g_img;
}

void splash_bg_gif_stop(void)
{
    if (g_gif != nullptr) {
        g_gif->Stop();
        g_gif.reset();
    }
    g_img = nullptr;
}

} /* extern "C" */
