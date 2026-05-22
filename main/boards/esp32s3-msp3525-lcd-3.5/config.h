#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>
#include <driver/spi_master.h>

/* MSP3525/MSP3526 3.5" SPI module — pins from LVGL_Demos/User_Setup.h & touch.h
 * LCD: ST7796S 320x480, touch: FT6336 (I2C 0x38) per user manual */

#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

#define BUILTIN_LED_GPIO        GPIO_NUM_NC
#define BOOT_BUTTON_GPIO        GPIO_NUM_0

/* ST7796 SPI (User_Setup.h) */
#define DISPLAY_SPI_MODE        0
#define DISPLAY_SPI_HOST        SPI2_HOST
#define DISPLAY_MOSI_PIN        GPIO_NUM_13
#define DISPLAY_CLK_PIN         GPIO_NUM_14
#define DISPLAY_CS_PIN          GPIO_NUM_15
#define DISPLAY_DC_PIN          GPIO_NUM_2
#define DISPLAY_RST_PIN         GPIO_NUM_16
#define DISPLAY_BACKLIGHT_PIN   GPIO_NUM_21
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false

/* Landscape UI 480x320 (LVGL_Demos.ino setRotation(1)) */
#define DISPLAY_WIDTH           480
#define DISPLAY_HEIGHT          320
#define DISPLAY_MIRROR_X        false
#define DISPLAY_MIRROR_Y        false
#define DISPLAY_SWAP_XY         true
#define DISPLAY_RGB_ORDER       LCD_RGB_ELEMENT_ORDER_BGR
#define DISPLAY_INVERT_COLOR    true
#define DISPLAY_OFFSET_X        0
#define DISPLAY_OFFSET_Y        0

/* FT6336 touch (touch.h) */
#define TOUCH_I2C_SDA_PIN       GPIO_NUM_38
#define TOUCH_I2C_SCL_PIN       GPIO_NUM_37
#define TOUCH_RST_PIN           GPIO_NUM_35
#define TOUCH_INT_PIN           GPIO_NUM_39

/* Touch raw coordinate range (portrait panel, touch.h TOUCH_MAP_*) */
#define TOUCH_RAW_X_MIN         0
#define TOUCH_RAW_X_MAX         320
#define TOUCH_RAW_Y_MIN         0
#define TOUCH_RAW_Y_MAX         480

/* No external audio codec */
#define AUDIO_I2S_GPIO_WS       GPIO_NUM_4
#define AUDIO_I2S_GPIO_BCLK     GPIO_NUM_5
#define AUDIO_I2S_GPIO_DIN      GPIO_NUM_6
#define AUDIO_I2S_GPIO_DOUT     GPIO_NUM_7

#endif // _BOARD_CONFIG_H_
