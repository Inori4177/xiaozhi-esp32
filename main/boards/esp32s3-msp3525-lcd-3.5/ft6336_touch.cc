#include "ft6336_touch.h"
#include "config.h"

#include <esp_log.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "ft6336_touch"

#define FT6336_REG_TD_STATUS   0x02
#define FT6336_REG_TOUCH1      0x03

#define FT6336_REG_FOCALTECH_ID 0xA8
#define FT6336_REG_CIPHER_MID   0x9F
#define FT6336_REG_CIPHER_HIGH  0xA3
#define FT6336_REG_THGROUP      0x80
#define FT6336_REG_PERIODACTIVE 0x88
#define FT6336_REG_G_MODE       0xA4

static i2c_master_dev_handle_t dev_handle = nullptr;
static uint16_t g_width = 0;
static uint16_t g_height = 0;
static uint8_t g_rotation = 0;
static int g_min_x = 0, g_max_x = 0, g_min_y = 0, g_max_y = 0;
static int g_last_x = 0, g_last_y = 0;
static bool g_touched = false;

static esp_err_t read_reg(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(dev_handle, &reg, 1, data, len, pdMS_TO_TICKS(100));
}

static esp_err_t write_reg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    return i2c_master_transmit(dev_handle, buf, sizeof(buf), pdMS_TO_TICKS(100));
}

static void ft6336_apply_tuning(void)
{
    if (write_reg(FT6336_REG_THGROUP, TOUCH_THGROUP) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set THGROUP");
    }
    if (write_reg(FT6336_REG_PERIODACTIVE, TOUCH_PERIODACTIVE) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set PERIODACTIVE");
    }
#if TOUCH_USE_INTERRUPT
    /* G_MODE=1 trigger：每次触摸/坐标更新产生 INT 脉冲，配合 LV_INDEV_MODE_EVENT */
    if (write_reg(FT6336_REG_G_MODE, TOUCH_G_MODE_TRIGGER) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set G_MODE trigger");
    }
#endif
    ESP_LOGI(TAG, "Touch tuning THGROUP=0x%02x PERIODACTIVE=%d G_MODE=%d",
             TOUCH_THGROUP, TOUCH_PERIODACTIVE,
#if TOUCH_USE_INTERRUPT
             TOUCH_G_MODE_TRIGGER
#else
             0
#endif
             );
}

static bool ft6336_reset_check(void)
{
    if (TOUCH_RST_PIN != GPIO_NUM_NC) {
        gpio_config_t rst_cfg = {
            .pin_bit_mask = 1ULL << TOUCH_RST_PIN,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&rst_cfg);
        gpio_set_level(TOUCH_RST_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(20));
        gpio_set_level(TOUCH_RST_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(20));
        gpio_set_level(TOUCH_RST_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    uint8_t tmp[2] = {0};
    if (read_reg(FT6336_REG_FOCALTECH_ID, tmp, 1) != ESP_OK || tmp[0] != 0x11) {
        ESP_LOGE(TAG, "FT6336 chip id mismatch: 0x%02x", tmp[0]);
        return false;
    }
    if (read_reg(FT6336_REG_CIPHER_MID, tmp, 2) != ESP_OK || tmp[0] != 0x26) {
        ESP_LOGE(TAG, "FT6336 cipher mid mismatch");
        return false;
    }
    if (read_reg(FT6336_REG_CIPHER_HIGH, tmp, 1) != ESP_OK || tmp[0] != 0x64) {
        ESP_LOGE(TAG, "FT6336 cipher high mismatch");
        return false;
    }
    return true;
}

static int map_coord(int value, int in_min, int in_max, int out_max)
{
    if (in_max <= in_min) {
        return 0;
    }
    const int margin = TOUCH_MAP_MARGIN;
    in_min -= margin;
    in_max += margin;
    value = value < in_min ? in_min : (value > in_max ? in_max : value);
    return (value - in_min) * out_max / (in_max - in_min);
}

void ft6336_touch_init(i2c_master_bus_handle_t bus_handle, uint16_t width, uint16_t height, uint8_t rotation)
{
    g_width = width;
    g_height = height;
    g_rotation = rotation;

    switch (rotation) {
        case FT6336_ROTATION_LEFT:
        case FT6336_ROTATION_RIGHT:
            g_min_x = TOUCH_RAW_Y_MIN;
            g_max_x = TOUCH_RAW_Y_MAX;
            g_min_y = TOUCH_RAW_X_MIN;
            g_max_y = TOUCH_RAW_X_MAX;
            break;
        default:
            g_min_x = TOUCH_RAW_X_MIN;
            g_max_x = TOUCH_RAW_X_MAX;
            g_min_y = TOUCH_RAW_Y_MIN;
            g_max_y = TOUCH_RAW_Y_MAX;
            break;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = FT6336_I2C_ADDR,
        .scl_speed_hz = 400000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    if (!ft6336_reset_check()) {
        ESP_LOGW(TAG, "FT6336 init check failed, touch may not work");
        ESP_LOGW(TAG, "Check CTP: SDA=GPIO%d SCL=GPIO%d RST=GPIO%d (not 26~37)",
                 (int)TOUCH_I2C_SDA_PIN, (int)TOUCH_I2C_SCL_PIN, (int)TOUCH_RST_PIN);
    } else {
        ESP_LOGI(TAG, "FT6336 initialized: logical %ux%u rot=%u map X[%d,%d] Y[%d,%d]",
                 (unsigned)g_width, (unsigned)g_height, (unsigned)g_rotation,
                 g_min_x, g_max_x, g_min_y, g_max_y);
        ft6336_apply_tuning();
    }
}

void ft6336_touch_read(void)
{
    uint8_t status = 0;
    uint8_t data[4] = {0};

    g_touched = false;
    if (read_reg(FT6336_REG_TD_STATUS, &status, 1) != ESP_OK) {
        return;
    }

    uint8_t touches = status & 0x0F;
    /* 与 Arduino 一致：1~2 点；若低 4 位为 0 但 status>0，按 1 点处理（部分批次固件） */
    if (touches == 0) {
        if (status == 0) {
            return;
        }
        touches = 1;
    } else if (touches > 2) {
        return;
    }

    if (read_reg(FT6336_REG_TOUCH1, data, 4) != ESP_OK) {
        return;
    }

    uint16_t raw_x = ((data[0] & 0x0F) << 8) | data[1];
    uint16_t raw_y = ((data[2] & 0x0F) << 8) | data[3];

    /* Match FT6336-arduino/FT6336.cpp readPoint(): rotation uses native panel
     * size (320x480 from TOUCH_MAP_*), NOT logical LVGL size (480x320). */
    uint16_t tx = raw_x;
    uint16_t ty = raw_y;
    const uint16_t native_w = TOUCH_RAW_X_MAX;
    const uint16_t native_h = TOUCH_RAW_Y_MAX;
    switch (g_rotation) {
        case FT6336_ROTATION_LEFT:
            tx = native_h - raw_y;
            ty = raw_x;
            break;
        case FT6336_ROTATION_INVERTED:
            tx = native_w - raw_x;
            ty = native_h - raw_y;
            break;
        case FT6336_ROTATION_RIGHT: /* TFT setRotation(1) / LVGL_Demos.ino */
            tx = raw_y;
            ty = native_w - raw_x;
            break;
        default:
            break;
    }

    g_last_x = map_coord(static_cast<int>(tx), g_min_x, g_max_x, static_cast<int>(g_width) - 1);
    g_last_y = map_coord(static_cast<int>(ty), g_min_y, g_max_y, static_cast<int>(g_height) - 1);
#if TOUCH_INVERT_X
    if (g_width > 0) {
        g_last_x = static_cast<int>(g_width) - 1 - g_last_x;
    }
#endif
#if TOUCH_INVERT_Y
    if (g_height > 0) {
        g_last_y = static_cast<int>(g_height) - 1 - g_last_y;
    }
#endif
    g_touched = true;
}

bool ft6336_touch_get_point(int *x, int *y)
{
    if (!g_touched || x == nullptr || y == nullptr) {
        return false;
    }
    *x = g_last_x;
    *y = g_last_y;
    return true;
}

bool ft6336_touch_is_pressed(void)
{
    return g_touched;
}
