#include "laser_gcode.h"
#include "config.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include <driver/uart.h>
#include <esp_log.h>

static const char *TAG = "laser_gcode";

static bool g_uart_ready = false;

void laser_gcode_init(void)
{
#if defined(LASER_UART_TX_PIN) && (LASER_UART_TX_PIN != GPIO_NUM_NC)
    uart_config_t cfg = {
        .baud_rate = LASER_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(LASER_UART_NUM, 512, 512, 0, nullptr, 0));
    ESP_ERROR_CHECK(uart_param_config(LASER_UART_NUM, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(LASER_UART_NUM, LASER_UART_TX_PIN, LASER_UART_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    g_uart_ready = true;
    ESP_LOGI(TAG, "UART%d TX=GPIO%d baud=%d", LASER_UART_NUM, (int)LASER_UART_TX_PIN,
             LASER_UART_BAUD_RATE);
#else
    ESP_LOGW(TAG, "UART TX not configured; G-code will be logged only");
#endif
}

void laser_gcode_send(const char *line)
{
    if (line == nullptr || line[0] == '\0') {
        return;
    }
    ESP_LOGI(TAG, ">> %s", line);
    if (!g_uart_ready) {
        return;
    }
    uart_write_bytes(LASER_UART_NUM, line, strlen(line));
    uart_write_bytes(LASER_UART_NUM, "\n", 1);
}

void laser_gcode_sendf(const char *fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    laser_gcode_send(buf);
}
