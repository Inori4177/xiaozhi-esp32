#include "lvgl_image.h"
#include <cbin_font.h>

#include <esp_log.h>
#include <stdexcept>
#include <cstring>
#include <esp_heap_caps.h>

#define TAG "LvglImage"


LvglRawImage::LvglRawImage(void* data, size_t size) {
    bzero(&image_dsc_, sizeof(image_dsc_));
    image_dsc_.data_size = size;
    image_dsc_.data = static_cast<uint8_t*>(data);
    image_dsc_.header.magic = LV_IMAGE_HEADER_MAGIC;
    image_dsc_.header.cf = LV_COLOR_FORMAT_RAW_ALPHA;
    image_dsc_.header.w = 0;
    image_dsc_.header.h = 0;
}

bool LvglRawImage::IsGif() const {
    auto ptr = (const uint8_t*)image_dsc_.data;
    return ptr[0] == 'G' && ptr[1] == 'I' && ptr[2] == 'F';
}

LvglCBinImage::LvglCBinImage(void* data) {
    image_dsc_ = cbin_img_dsc_create(static_cast<uint8_t*>(data));
}

LvglCBinImage::~LvglCBinImage() {
    if (image_dsc_ != nullptr) {
        cbin_img_dsc_delete(image_dsc_);
    }
}

LvglAllocatedImage::LvglAllocatedImage(void* data, size_t size) {
    bzero(&image_dsc_, sizeof(image_dsc_));
    image_dsc_.data_size = size;
    image_dsc_.data = static_cast<uint8_t*>(data);

    if (lv_image_decoder_get_info(&image_dsc_, &image_dsc_.header) != LV_RESULT_OK) {
        ESP_LOGE(TAG, "Failed to get image info, data: %p size: %u", data, size);
        throw std::runtime_error("Failed to get image info");
    }
}

LvglAllocatedImage::LvglAllocatedImage(void* data, size_t size, int width, int height, int stride, int color_format) {
    bzero(&image_dsc_, sizeof(image_dsc_));
    image_dsc_.data_size = size;
    image_dsc_.data = static_cast<uint8_t*>(data);
    image_dsc_.header.magic = LV_IMAGE_HEADER_MAGIC;
    image_dsc_.header.cf = color_format;
    image_dsc_.header.w = width;
    image_dsc_.header.h = height;
    image_dsc_.header.stride = stride;
}

LvglAllocatedImage::~LvglAllocatedImage() {
    if (image_dsc_.data) {
        heap_caps_free((void*)image_dsc_.data);
        image_dsc_.data = nullptr;
    }
}

LvglBorrowedImage::LvglBorrowedImage(void* data, size_t size) {
    bzero(&image_dsc_, sizeof(image_dsc_));
    if (data == nullptr || size <= sizeof(lv_image_header_t)) {
        ESP_LOGE(TAG, "Invalid borrowed image buffer, data: %p size: %u", data, size);
        throw std::runtime_error("Invalid borrowed image buffer");
    }

    lv_image_header_t header;
    memcpy(&header, data, sizeof(header));
    if (header.magic != LV_IMAGE_HEADER_MAGIC) {
        ESP_LOGE(TAG, "Invalid borrowed image magic, data: %p size: %u magic: %u", data, size,
                 (unsigned)header.magic);
        throw std::runtime_error("Invalid borrowed image magic");
    }

    image_dsc_.header = header;
    image_dsc_.data_size = size - sizeof(lv_image_header_t);
    image_dsc_.data = static_cast<const uint8_t*>(data) + sizeof(lv_image_header_t);
}
