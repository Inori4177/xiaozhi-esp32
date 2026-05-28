#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>
#include <driver/spi_master.h>

/* Bread Compact WiFi + MSP3525 3.5" ST7796 + FT6336
 *
 * 以下引脚与原版 bread-compact-wifi 保持一致，请勿改动：
 *   I2S: MIC WS/SCK/DIN=4/5/6, SPK DOUT/BCLK/LRCK=7/15/16
 *   按键/LED: BOOT=0, VOL+=40, VOL-=39, TOUCH_BTN=47, LED=48, LAMP=18
 *
 * 触摸屏相对 MSP3525 参考接线仅改 3 根（模块排针重接 ESP32）：
 *   CTP_SDA: 8 -> 3  （复用原 OLED I2C_SDA）
 *   CTP_SCL: 9 -> 8  （复用原 OLED I2C_SCL）
 *   CTP_INT: 16 -> 9 （避开 I2S LRCK=16）
 *   CTP_RST 仍为 10
 *
 * LCD_CS 使用 GPIO1（避开 I2S BCLK=15），其余 SPI 与参考板一致。
 */

#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

// 如果使用 Duplex I2S 模式，请注释下面一行
#define AUDIO_I2S_METHOD_SIMPLEX

#ifdef AUDIO_I2S_METHOD_SIMPLEX

#define AUDIO_I2S_MIC_GPIO_WS   GPIO_NUM_4
#define AUDIO_I2S_MIC_GPIO_SCK  GPIO_NUM_5
#define AUDIO_I2S_MIC_GPIO_DIN  GPIO_NUM_6
#define AUDIO_I2S_SPK_GPIO_DOUT GPIO_NUM_7
#define AUDIO_I2S_SPK_GPIO_BCLK GPIO_NUM_15
#define AUDIO_I2S_SPK_GPIO_LRCK GPIO_NUM_16

#else

#define AUDIO_I2S_GPIO_WS       GPIO_NUM_4
#define AUDIO_I2S_GPIO_BCLK     GPIO_NUM_5
#define AUDIO_I2S_GPIO_DIN      GPIO_NUM_6
#define AUDIO_I2S_GPIO_DOUT     GPIO_NUM_7

#endif

#define BUILTIN_LED_GPIO        GPIO_NUM_48
#define BOOT_BUTTON_GPIO        GPIO_NUM_0
#define TOUCH_BUTTON_GPIO       GPIO_NUM_47
#define VOLUME_UP_BUTTON_GPIO   GPIO_NUM_40
#define VOLUME_DOWN_BUTTON_GPIO GPIO_NUM_39

/* 原 OLED I2C 引脚，现供 FT6336 使用 */
#define DISPLAY_SDA_PIN         GPIO_NUM_3
#define DISPLAY_SCL_PIN         GPIO_NUM_8

/* ST7796 SPI */
#define DISPLAY_SPI_MODE        0
#define DISPLAY_SPI_CLOCK_HZ    (40 * 1000 * 1000)
#define DISPLAY_SPI_HOST        SPI2_HOST
#define DISPLAY_MOSI_PIN        GPIO_NUM_13
#define DISPLAY_MISO_PIN        GPIO_NUM_12
#define DISPLAY_CLK_PIN         GPIO_NUM_14
#define DISPLAY_CS_PIN          GPIO_NUM_1
#define DISPLAY_DC_PIN          GPIO_NUM_2
#define DISPLAY_RST_PIN         GPIO_NUM_11
#define DISPLAY_BACKLIGHT_PIN   GPIO_NUM_21
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false

#define DISPLAY_WIDTH           480
#define DISPLAY_HEIGHT          320

/* LVGL / esp_lvgl_port 内存（乐鑫 FAQ + esp_lvgl_port PSRAM canvas 方案）
 * - DRAW_BUF_LINES: 条带高度，≥ 屏面积 10%（480×32=15360 px）以保证帧率
 * - TRANS_BUF_LINES: SPI DMA 搬运条带，放 SRAM，减轻 PSRAM→SRAM 临时拷贝
 * - 激光 UI 下 lcd_display.cc 使用单缓冲 + trans_size，见 CONFIG_MSP3525_LASER_UI */
#define MSP3525_LVGL_DRAW_BUF_LINES   32
#define MSP3525_LVGL_TRANS_BUF_LINES  20
#define DISPLAY_MIRROR_X        false
#define DISPLAY_MIRROR_Y        false
#define DISPLAY_SWAP_XY         true
#define DISPLAY_RGB_ORDER       LCD_RGB_ELEMENT_ORDER_BGR
#define DISPLAY_INVERT_COLOR    true
#define DISPLAY_OFFSET_X        0
#define DISPLAY_OFFSET_Y        0

/* FT6336 — SDA/SCL 接原 OLED 总线；INT 接 GPIO9 */
#define TOUCH_I2C_SDA_PIN       DISPLAY_SDA_PIN
#define TOUCH_I2C_SCL_PIN       DISPLAY_SCL_PIN
#define TOUCH_RST_PIN           GPIO_NUM_10
#define TOUCH_INT_PIN           GPIO_NUM_9
#define TOUCH_USE_INTERRUPT     1
#define TOUCH_G_MODE_TRIGGER    1
#define TOUCH_RAW_X_MIN         0
#define TOUCH_RAW_X_MAX         320
#define TOUCH_RAW_Y_MIN         0
#define TOUCH_RAW_Y_MAX         480
#define TOUCH_THGROUP           0x02
#define TOUCH_PERIODACTIVE      1
#define TOUCH_MAP_MARGIN        24
#define TOUCH_INVERT_X          false
#define TOUCH_INVERT_Y          false

/* Laser / GRBL serial */
#define LASER_UART_NUM          UART_NUM_1
#define LASER_UART_TX_PIN       GPIO_NUM_17
#define LASER_UART_RX_PIN       GPIO_NUM_NC
#define LASER_UART_BAUD_RATE    115200

// A MCP Test: Control a lamp
#define LAMP_GPIO GPIO_NUM_18

#endif // _BOARD_CONFIG_H_
