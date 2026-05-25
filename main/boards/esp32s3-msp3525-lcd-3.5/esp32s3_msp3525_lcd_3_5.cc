#include "wifi_board.h"
#include "backlight.h"
#include "codecs/no_audio_codec.h"
#include "display/lcd_display.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "board_custom_ui.h"
#include "ft6336_touch.h"

#include <esp_log.h>
#include <driver/i2c_master.h>
#include <driver/spi_common.h>
#include <driver/gpio.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_st7796.h>
#include <esp_lvgl_port.h>
#include <esp_timer.h>
#include <lvgl.h>

#define TAG "msp3525_lcd_3_5"

static lv_indev_t *g_touch_indev = nullptr;
static esp_timer_handle_t g_touch_poll_timer = nullptr;
static bool g_last_touch_pressed = false;

static void touch_wake_lvgl(void)
{
    if (g_touch_indev != nullptr) {
        lvgl_port_task_wake(LVGL_PORT_EVENT_TOUCH, g_touch_indev);
    }
}

static void touch_poll_timer_cb(void *arg)
{
    (void)arg;
    ft6336_touch_read();
    const bool pressed = ft6336_touch_is_pressed();
    if (pressed != g_last_touch_pressed || pressed) {
        g_last_touch_pressed = pressed;
        touch_wake_lvgl();
    }
}

static void IRAM_ATTR touch_int_isr(void *arg)
{
    (void)arg;
    touch_wake_lvgl();
}

static void touch_start_polling(void)
{
    if (g_touch_poll_timer != nullptr) {
        return;
    }
    const esp_timer_create_args_t args = {
        .callback = &touch_poll_timer_cb,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "touch_poll",
        .skip_unhandled_events = true,
    };
    if (esp_timer_create(&args, &g_touch_poll_timer) == ESP_OK) {
        esp_timer_start_periodic(g_touch_poll_timer, 10 * 1000);
        ESP_LOGI(TAG, "Touch poll 10ms (wake LVGL on touch)");
    }
}

static void touch_setup_interrupt(void)
{
    if (TOUCH_INT_PIN == GPIO_NUM_NC) {
        return;
    }
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << TOUCH_INT_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&cfg);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(TOUCH_INT_PIN, touch_int_isr, nullptr);
    ESP_LOGI(TAG, "Touch INT on GPIO%d", (int)TOUCH_INT_PIN);
}

/* LVGL_Demos/ST7796_Init.h — 不含 0x36/0x3A/0x21/0x29（由 esp_lcd API 统一管理，见乐鑫 SPI LCD 移植文档） */
static const st7796_lcd_init_cmd_t lcd_init_cmds[] = {
    {0xF0, (uint8_t[]){0xC3}, 1, 0},
    {0xF0, (uint8_t[]){0x96}, 1, 0},
    {0xB4, (uint8_t[]){0x02}, 1, 0},
    {0xB7, (uint8_t[]){0xC6}, 1, 0},
    {0xC0, (uint8_t[]){0xC0, 0x00}, 2, 0},
    {0xC1, (uint8_t[]){0x13}, 1, 0},
    {0xC2, (uint8_t[]){0xA7}, 1, 0},
    {0xC5, (uint8_t[]){0x21}, 1, 0},
    {0xE8, (uint8_t[]){0x40, 0x8A, 0x1B, 0x1B, 0x23, 0x0A, 0xAC, 0x33}, 8, 0},
    {0xE0, (uint8_t[]){0xD2, 0x05, 0x08, 0x06, 0x05, 0x02, 0x2A, 0x44, 0x46, 0x39, 0x15, 0x15, 0x2D, 0x32}, 14, 0},
    {0xE1, (uint8_t[]){0x96, 0x08, 0x0C, 0x09, 0x09, 0x25, 0x2E, 0x43, 0x42, 0x35, 0x11, 0x11, 0x28, 0x2E}, 14, 0},
    {0xF0, (uint8_t[]){0x3C}, 1, 0},
    {0xF0, (uint8_t[]){0x69}, 1, 120},
};

static void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    static int last_x = 0;
    static int last_y = 0;
    int x = 0;
    int y = 0;

    /* 坐标由 10ms 轮询更新，此处不再重复 I2C，降低延迟与丢点 */
    if (ft6336_touch_get_point(&x, &y)) {
        last_x = x;
        last_y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
    data->point.x = last_x;
    data->point.y = last_y;
}

/* Arduino User_Setup: TFT_BACKLIGHT_ON HIGH — GPIO 常亮，立即生效 */
class GpioBacklight : public Backlight {
public:
    GpioBacklight(gpio_num_t pin, bool output_invert) : Backlight(), pin_(pin), output_invert_(output_invert) {
        gpio_config_t cfg = {
            .pin_bit_mask = 1ULL << pin,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&cfg);
        SetBrightnessImpl(100);
    }

    void SetBrightnessImpl(uint8_t brightness) override {
        const int on = brightness > 0;
        gpio_set_level(pin_, output_invert_ ? !on : on);
    }

private:
    gpio_num_t pin_;
    bool output_invert_;
};

class Esp32s3Msp3525Lcd35Board : public WifiBoard {
private:
    Button boot_button_;
    i2c_master_bus_handle_t i2c_bus_ = nullptr;
    LcdDisplay *display_ = nullptr;

    void InitializeLcdResetPin() {
        if (DISPLAY_RST_PIN != GPIO_NUM_NC) {
            gpio_config_t cfg = {
                .pin_bit_mask = 1ULL << DISPLAY_RST_PIN,
                .mode = GPIO_MODE_OUTPUT,
                .pull_up_en = GPIO_PULLUP_DISABLE,
                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                .intr_type = GPIO_INTR_DISABLE,
            };
            gpio_config(&cfg);
            gpio_set_level(DISPLAY_RST_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(10));
            gpio_set_level(DISPLAY_RST_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(20));
            gpio_set_level(DISPLAY_RST_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(120));
        }
    }

    void InitializeI2c() {
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = TOUCH_I2C_SDA_PIN,
            .scl_io_num = TOUCH_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &i2c_bus_));
    }

    void InitializeSpi() {
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = DISPLAY_MOSI_PIN;
        buscfg.miso_io_num = GPIO_NUM_NC;
        buscfg.sclk_io_num = DISPLAY_CLK_PIN;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(DISPLAY_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    void InitializeLcdDisplay() {
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;

        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = DISPLAY_CS_PIN;
        io_config.dc_gpio_num = DISPLAY_DC_PIN;
        io_config.spi_mode = DISPLAY_SPI_MODE;
        io_config.pclk_hz = DISPLAY_SPI_CLOCK_HZ;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(DISPLAY_SPI_HOST, &io_config, &panel_io));

        st7796_vendor_config_t vendor_config = {
            .init_cmds = lcd_init_cmds,
            .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(st7796_lcd_init_cmd_t),
        };

        const esp_lcd_panel_dev_config_t panel_config = {
            .reset_gpio_num = DISPLAY_RST_PIN,
            .rgb_ele_order = DISPLAY_RGB_ORDER,
            .bits_per_pixel = 16,
            .vendor_config = &vendor_config,
        };

        ESP_LOGI(TAG, "Install ST7796 panel driver");
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7796(panel_io, &panel_config, &panel));

        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel, DISPLAY_INVERT_COLOR));
        /* 在 SpiLcdDisplay 白屏测试前设置 MADCTL（对齐 bread-compact / 乐鑫示例） */
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y));
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));

        display_ = new SpiLcdDisplay(panel_io, panel, DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                     DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y,
                                     DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
    }

    void InitializeTouch() {
        /* rotation=1 matches LVGL_Demos: setRotation(1) -> FT6336 ROTATION_RIGHT */
        ft6336_touch_init(i2c_bus_, DISPLAY_WIDTH, DISPLAY_HEIGHT, FT6336_ROTATION_RIGHT);

        g_touch_indev = lv_indev_create();
        lv_indev_set_type(g_touch_indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(g_touch_indev, touchpad_read);
        lv_indev_set_display(g_touch_indev, lv_display_get_default());
        /* EVENT：由轮询/中断唤醒 LVGL 后再读缓存坐标 */
        lv_indev_set_mode(g_touch_indev, LV_INDEV_MODE_EVENT);

        touch_setup_interrupt();
        touch_start_polling();
    }

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto &app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
            app.ToggleChatState();
        });
    }

public:
    Esp32s3Msp3525Lcd35Board() : boot_button_(BOOT_BUTTON_GPIO) {
        InitializeLcdResetPin();
        GetBacklight()->SetBrightness(100);

        ESP_LOGI(TAG, "Init SPI / LCD");
        InitializeSpi();
        vTaskDelay(1);
        InitializeLcdDisplay();
        vTaskDelay(1);

        ESP_LOGI(TAG, "Init I2C / touch");
        InitializeI2c();
        vTaskDelay(1);
        InitializeTouch();
        vTaskDelay(1);

        InitializeButtons();
        GetBacklight()->RestoreBrightness();
        ESP_LOGI(TAG, "Board init done");
    }

    virtual AudioCodec *GetAudioCodec() override {
        static NoAudioCodecDuplex audio_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
                                              AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS,
                                              AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN);
        return &audio_codec;
    }

    virtual Display *GetDisplay() override {
        return display_;
    }

    virtual Backlight *GetBacklight() override {
        static GpioBacklight backlight(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
        return &backlight;
    }

#if CONFIG_MSP3525_LASER_UI
    virtual const BoardCustomUiOps *GetCustomUiOps() override {
        return Msp3525GetLaserUiOps();
    }
#endif
};

DECLARE_BOARD(Esp32s3Msp3525Lcd35Board);
