#include "peer_uart_link.h"

#include "config.h"

#include <sdkconfig.h>

#include <cstring>

#include <driver/uart.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

static const char *TAG = "peer_uart";

#ifndef PEER_UART_RX_BUF
#define PEER_UART_RX_BUF 2048
#endif
#ifndef PEER_UART_TX_BUF
#define PEER_UART_TX_BUF 1024
#endif
#ifndef PEER_UART_LINE_MAX
#define PEER_UART_LINE_MAX 512
#endif

static SemaphoreHandle_t s_tx_mutex = nullptr;
static peer_uart_line_cb_t s_line_cb = nullptr;
static void *s_line_user = nullptr;
static volatile bool s_link_up = false;
static TaskHandle_t s_rx_task = nullptr;

static bool peer_uart_write_raw(const char *data, size_t len)
{
    if (data == nullptr || len == 0) {
        return true;
    }
    const int n = uart_write_bytes(PEER_UART_NUM, data, len);
    return n == static_cast<int>(len);
}

bool peer_uart_link_send_line(const char *line)
{
    if (line == nullptr || s_tx_mutex == nullptr) {
        return false;
    }
    if (xSemaphoreTake(s_tx_mutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        ESP_LOGW(TAG, "TX mutex timeout");
        return false;
    }
    const bool ok = peer_uart_write_raw(line, strlen(line)) && peer_uart_write_raw("\n", 1);
    xSemaphoreGive(s_tx_mutex);
    return ok;
}

bool peer_uart_link_is_up(void)
{
    return s_link_up;
}

void peer_uart_link_set_line_callback(peer_uart_line_cb_t cb, void *user_data)
{
    s_line_cb = cb;
    s_line_user = user_data;
}

static void rx_task(void *arg)
{
    (void)arg;
    char line[PEER_UART_LINE_MAX];
    size_t len = 0;

    for (;;) {
        uint8_t byte = 0;
        const int n = uart_read_bytes(PEER_UART_NUM, &byte, 1, pdMS_TO_TICKS(100));
        if (n <= 0) {
            continue;
        }
        if (byte == '\r') {
            continue;
        }
        if (byte == '\n') {
            if (len == 0) {
                continue;
            }
            line[len] = '\0';
            if (s_line_cb != nullptr) {
                s_line_cb(line, s_line_user);
            }
            len = 0;
            continue;
        }
        if (len + 1 >= sizeof(line)) {
            ESP_LOGW(TAG, "RX line overflow, discarding");
            len = 0;
            continue;
        }
        line[len++] = static_cast<char>(byte);
    }
}

bool peer_uart_link_init(void)
{
    if (s_tx_mutex != nullptr) {
        return true;
    }

    uart_config_t cfg = {};
    cfg.baud_rate = CONFIG_INTERACTION_PEER_UART_BAUD;
    cfg.data_bits = UART_DATA_8_BITS;
    cfg.parity = UART_PARITY_DISABLE;
    cfg.stop_bits = UART_STOP_BITS_1;
    cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    cfg.source_clk = UART_SCLK_DEFAULT;

    ESP_ERROR_CHECK(uart_driver_install(PEER_UART_NUM, PEER_UART_RX_BUF, PEER_UART_TX_BUF, 0, nullptr, 0));
    ESP_ERROR_CHECK(uart_param_config(PEER_UART_NUM, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(PEER_UART_NUM, PEER_UART_TX_PIN, PEER_UART_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    s_tx_mutex = xSemaphoreCreateMutex();
    if (s_tx_mutex == nullptr) {
        return false;
    }

    if (xTaskCreatePinnedToCore(rx_task, "peer_uart_rx", 4096, nullptr, 5, &s_rx_task, 0) != pdPASS) {
        ESP_LOGE(TAG, "RX task create failed");
        return false;
    }

    s_link_up = true;
    ESP_LOGI(TAG, "Peer UART%d TX=%d RX=%d @ %d baud",
             static_cast<int>(PEER_UART_NUM), static_cast<int>(PEER_UART_TX_PIN),
             static_cast<int>(PEER_UART_RX_PIN), CONFIG_INTERACTION_PEER_UART_BAUD);
    return true;
}
