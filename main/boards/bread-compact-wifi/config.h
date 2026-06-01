#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>
#include <driver/spi_master.h>

/* Bread Compact WiFi + MSP3525 3.5" ST7796 + FT6336
 *
 * 交互 S3（本固件）：屏 + 触摸 + WebUI；通过 UART 连接运动/语音 S3。
 * 运动 S3：I2S 语音 + CNC 步进（GPIO 9/10/11/12/13/17 等）。
 *
 * 双机 UART（交叉连接，共 GND）：
 *   交互 TX GPIO17 -> 运动 RX
 *   交互 RX GPIO9  <- 运动 TX
 *
 * 屏幕 SPI 仅用普通 GPIO（乐鑫 P2，避开 USB-JTAG/JTAG/UART0/PSRAM）：
 *   MOSI=38, MISO=3, SCK=14, CS=1, DC=2, RST=21, BL=8
 *   MISO 接模块 SDO，用于读寄存器/抓屏（esp_lcd_panel_io_rx 等）
 *   GPIO3 为 Strapping(JTAG 源)，上电时 SDO 须高阻/弱上拉，复位后作 SPI 读
 *
 * 触摸 I2C 同样避开特殊脚；RST/INT 用 JTAG 脚（仅触摸控制，非 LCD 信号）：
 *   CTP_SDA=47, CTP_SCL=48, CTP_RST=41, CTP_INT=42
 *
 * 小智按键/LED 可改线，占用 JTAG/USB 脚即可（不影响屏/CNC）：
 *   BOOT=0, VOL+=39, VOL-=40, TOUCH_BTN=19, LED=20
 *
 * 禁止占用：GPIO26~37, 43/44(UART0), 45/46(Strapping)
 *
 * 模块排针改线一览：
 *   LCD_SDI=38  LCD_SDO=3  LCD_SCK=14  LCD_CS=1  LCD_RS=2  LCD_RST=21  LCD_LED=8
 *   CTP_SDA=47  CTP_SCL=48  CTP_RST=41  CTP_INT=42
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

#define BUILTIN_LED_GPIO        GPIO_NUM_20
#define BOOT_BUTTON_GPIO        GPIO_NUM_0
#define TOUCH_BUTTON_GPIO       GPIO_NUM_19
#define VOLUME_UP_BUTTON_GPIO   GPIO_NUM_40
#define VOLUME_DOWN_BUTTON_GPIO GPIO_NUM_39

/* ST7796 SPI — 全部使用普通 GPIO，不占 Strapping/USB/JTAG/UART0/PSRAM */
#define DISPLAY_SPI_MODE        0
#define DISPLAY_SPI_CLOCK_HZ    (40 * 1000 * 1000)
#define DISPLAY_SPI_HOST        SPI2_HOST
#define DISPLAY_MOSI_PIN        GPIO_NUM_38
#define DISPLAY_MISO_PIN        GPIO_NUM_3
#define DISPLAY_CLK_PIN         GPIO_NUM_14
#define DISPLAY_CS_PIN          GPIO_NUM_1
#define DISPLAY_DC_PIN          GPIO_NUM_2
#define DISPLAY_RST_PIN         GPIO_NUM_21
#define DISPLAY_BACKLIGHT_PIN   GPIO_NUM_8
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false

#define DISPLAY_WIDTH           480
#define DISPLAY_HEIGHT          320

/* LVGL / esp_lvgl_port 内存（乐鑫 FAQ + esp_lvgl_port PSRAM canvas 方案）
 * - DRAW_BUF_LINES: LVGL 条带；激光 UI 用 internal DMA（约 480×16×2≈15KB），避免 PSRAM 刷屏 priv 拷贝
 * - TRANS_BUF_LINES: 仅 PSRAM canvas 方案使用；激光 UI 下为 0 */
#define MSP3525_LVGL_DRAW_BUF_LINES   16
#define MSP3525_LVGL_TRANS_BUF_LINES  0
/** 激光 UI 控件多 + async 回调，taskLVGL 需更大栈（默认 8KB 易溢出） */
#define MSP3525_LVGL_TASK_STACK       (12 * 1024)
#define DISPLAY_MIRROR_X        false
#define DISPLAY_MIRROR_Y        false
#define DISPLAY_SWAP_XY         true
#define DISPLAY_RGB_ORDER       LCD_RGB_ELEMENT_ORDER_BGR
#define DISPLAY_INVERT_COLOR    true
#define DISPLAY_OFFSET_X        0
#define DISPLAY_OFFSET_Y        0

/* FT6336 — I2C 用普通 GPIO；RST/INT 用 JTAG 脚（非 LCD 信号） */
#define TOUCH_I2C_SDA_PIN       GPIO_NUM_47
#define TOUCH_I2C_SCL_PIN       GPIO_NUM_48
#define TOUCH_RST_PIN           GPIO_NUM_41
#define TOUCH_INT_PIN           GPIO_NUM_42
#define TOUCH_USE_INTERRUPT     1
#define TOUCH_G_MODE_TRIGGER    0
#define TOUCH_RAW_X_MIN         0
#define TOUCH_RAW_X_MAX         320
#define TOUCH_RAW_Y_MIN         0
#define TOUCH_RAW_Y_MAX         480
#define TOUCH_THGROUP           0x20
#define TOUCH_PERIODACTIVE      12
#define TOUCH_MAP_MARGIN        24
#define TOUCH_INVERT_X          false
#define TOUCH_INVERT_Y          false

/* UART link to motion/voice ESP32-S3 (NDJSON, see peer_link/peer_cnc_client.h) */
#define PEER_UART_NUM           UART_NUM_1
#define PEER_UART_TX_PIN        GPIO_NUM_17
#define PEER_UART_RX_PIN        GPIO_NUM_9

// A MCP Test: Control a lamp
#define LAMP_GPIO GPIO_NUM_18

#endif // _BOARD_CONFIG_H_
