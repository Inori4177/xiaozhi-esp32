#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>
#include <driver/spi_master.h>

/* MSP3525/MSP3526 3.5" — LVGL_Demos (ST7796 + FT6336)
 *
 * ESP32-S3-WROOM-1-N16R8 (8MB Octal PSRAM) 禁止占用 GPIO26~37（Flash/PSRAM 专用）。
 * LVGL_Demos/touch.h 里的 32/25/33 是给 ESP32 经典芯片用的，不能直接用于 S3 N16R8！
 *
 * 模块 14P 排针 -> ESP32-S3 推荐接线（与下方宏一致）：
 *   LCD_SDI(MOSI)=13  LCD_SCK=14  LCD_CS=15  LCD_RS(DC)=2
 *   LCD_RST=11      LCD_LED(BL)=21   (Arduino 示例为 GPIO27，S3 板请接 11)
 *   CTP_SDA=8       CTP_SCL=9       CTP_RST=10      CTP_INT=12
 */

#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

#define BUILTIN_LED_GPIO        GPIO_NUM_NC
#define BOOT_BUTTON_GPIO        GPIO_NUM_0

/* ST7796 SPI — from LVGL_Demos/User_Setup.h (safe on ESP32-S3) */
#define DISPLAY_SPI_MODE        0
#define DISPLAY_SPI_CLOCK_HZ    (40 * 1000 * 1000)
#define DISPLAY_SPI_HOST        SPI2_HOST
#define DISPLAY_MOSI_PIN        GPIO_NUM_13
#define DISPLAY_CLK_PIN         GPIO_NUM_14
#define DISPLAY_CS_PIN          GPIO_NUM_15
#define DISPLAY_DC_PIN          GPIO_NUM_2
#define DISPLAY_RST_PIN         GPIO_NUM_11
#define DISPLAY_BACKLIGHT_PIN   GPIO_NUM_21
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false

/* Landscape 480x320 — LVGL_Demos.ino setRotation(1)
 * TFT_eSPI ST7796_Rotation.h case 1: MV+BGR => swap_xy=true, mirror=false */
#define DISPLAY_WIDTH           480
#define DISPLAY_HEIGHT          320
#define DISPLAY_MIRROR_X        false
#define DISPLAY_MIRROR_Y        false
#define DISPLAY_SWAP_XY         true
#define DISPLAY_RGB_ORDER       LCD_RGB_ELEMENT_ORDER_BGR
#define DISPLAY_INVERT_COLOR    true
#define DISPLAY_OFFSET_X        0
#define DISPLAY_OFFSET_Y        0

/* FT6336 — 必须使用未占用 Flash/PSRAM 的 GPIO（勿用 26~37、32、33） */
#define TOUCH_I2C_SDA_PIN       GPIO_NUM_8
#define TOUCH_I2C_SCL_PIN       GPIO_NUM_9
#define TOUCH_RST_PIN           GPIO_NUM_10
/* CTP_INT：接 GPIO12（FT6336 低电平有效，任意边沿唤醒 LVGL） */
#define TOUCH_INT_PIN           GPIO_NUM_12

/* 原始坐标映射（可按实机微调，缩小死区） */
#define TOUCH_RAW_X_MIN         0
#define TOUCH_RAW_X_MAX         320
#define TOUCH_RAW_Y_MIN         0
#define TOUCH_RAW_Y_MAX         480

/* FT6336 灵敏度：THGROUP 越小越易触发；PERIODACTIVE 越小扫描越快（单位约 10ms） */
#define TOUCH_THGROUP           0x02
#define TOUCH_PERIODACTIVE      1

/* 无 INT 时轮询周期 (ms)；有 INT 时作后备采样 */
#define TOUCH_POLL_MS_NO_INT    10
#define TOUCH_POLL_MS_WITH_INT  8

/* 映射死区补偿：略扩大原始坐标范围，减轻边缘“按不到” */
#define TOUCH_MAP_MARGIN        24

/* 实机校准：旋转公式与 Arduino 一致后，若仍左右/上下颠倒可改为 1 */
#define TOUCH_INVERT_X          false
#define TOUCH_INVERT_Y          false

#define AUDIO_I2S_GPIO_WS       GPIO_NUM_4
#define AUDIO_I2S_GPIO_BCLK     GPIO_NUM_5
#define AUDIO_I2S_GPIO_DIN      GPIO_NUM_6
#define AUDIO_I2S_GPIO_DOUT     GPIO_NUM_7

#endif // _BOARD_CONFIG_H_
