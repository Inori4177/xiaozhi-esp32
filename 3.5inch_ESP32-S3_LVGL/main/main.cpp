#include <stdio.h>

#include "esp_log.h"
#include "bsp_i2c.h"
#include "bsp_display.h"
#include "bsp_touch.h"

#include "lv_port.h"

#include "demos/lv_demos.h"

static const char *TAG = "main";

#define EXAMPLE_DISPLAY_ROTATION LV_DISP_ROT_90
#define EXAMPLE_LCD_H_RES 320
#define EXAMPLE_LCD_V_RES 480
/* LVGL draw buffer: pixel count (recommend >= 1/10 screen, not full frame) */
#define LCD_DRAW_BUF_PIXELS (EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES / 10)
/* SPI max_transfer_sz unit: byte */
#define LCD_SPI_MAX_TRANSFER_SZ (EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * 2)

esp_lcd_panel_io_handle_t io_handle = NULL;
esp_lcd_panel_handle_t panel_handle = NULL;

lv_disp_drv_t disp_drv;

static lv_disp_t *lvgl_disp;
static lv_indev_t *lvgl_touch_indev = NULL;

void lv_port_init(void);


extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "app_main start");

    i2c_master_bus_handle_t i2c_bus_handle = bsp_i2c_init();
    ESP_LOGI(TAG, "I2C init done");

    bsp_display_init(&io_handle, &panel_handle, LCD_SPI_MAX_TRANSFER_SZ);
    ESP_LOGI(TAG, "display init done");

    bsp_display_brightness_init();
    bsp_display_set_brightness(100);
    ESP_LOGI(TAG, "backlight on");

    bsp_touch_init(i2c_bus_handle, EXAMPLE_LCD_V_RES, EXAMPLE_LCD_H_RES, 1);
    ESP_LOGI(TAG, "touch init done");

    lv_port_init();
    ESP_LOGI(TAG, "LVGL port init done");

    if (lvgl_port_lock(0)) {
        lv_demo_widgets();
        lvgl_port_unlock();
    }
    ESP_LOGI(TAG, "LVGL demo started");
}

static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    static lv_coord_t last_x = 0;
    static lv_coord_t last_y = 0;
    touch_data_t touch_data;
    /*Save the pressed coordinates and the state*/
    bsp_touch_read();
    if (bsp_touch_get_coordinates(&touch_data))
    {
        last_x = touch_data.coords[0].x;
        last_y = touch_data.coords[0].y;
        data->state = LV_INDEV_STATE_PR;
        //printf("x: %d, y: %d\n", last_x, last_y);
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
    /*Set the last pressed coordinates*/
    data->point.x = last_x;
    data->point.y = last_y;
}

void lv_port_init(void)
{
    lvgl_port_cfg_t port_cfg = {};

    port_cfg.task_priority = 4;
    port_cfg.task_stack = 1024 * 5;
    port_cfg.task_affinity = 1;
    port_cfg.task_max_sleep_ms = 500;
    port_cfg.timer_period_ms = 5;
    lvgl_port_init(&port_cfg);

    lvgl_port_display_cfg_t disp_cfg = {};
    disp_cfg.io_handle = io_handle;
    disp_cfg.panel_handle = panel_handle;
    disp_cfg.buffer_size = LCD_DRAW_BUF_PIXELS;
    disp_cfg.sw_rotate = EXAMPLE_DISPLAY_ROTATION;
    disp_cfg.hres = EXAMPLE_LCD_H_RES;
    disp_cfg.vres = EXAMPLE_LCD_V_RES;
    disp_cfg.trans_size = LCD_DRAW_BUF_PIXELS;
    disp_cfg.draw_wait_cb = NULL;
    disp_cfg.flags.buff_dma = false;
    disp_cfg.flags.buff_spiram = true;

    if (disp_cfg.sw_rotate == LV_DISP_ROT_180 || disp_cfg.sw_rotate == LV_DISP_ROT_NONE)
    {
        disp_cfg.hres = EXAMPLE_LCD_H_RES;
        disp_cfg.vres = EXAMPLE_LCD_V_RES;
    }
    else
    {
        disp_cfg.hres = EXAMPLE_LCD_V_RES;
        disp_cfg.vres = EXAMPLE_LCD_H_RES;
    }
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lvgl_touch_indev = lv_indev_drv_register(&indev_drv);
}
