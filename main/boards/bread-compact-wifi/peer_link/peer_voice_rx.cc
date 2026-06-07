#include "peer_voice_rx.h"
#include "peer_uart_mux.h"
#include "peer_voice_state.h"
#include "peer_voice_ui.h"

#include <sdkconfig.h>

#include <cJSON.h>
#include <cstring>

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "peer_voice_rx";

static void on_voice_line(const char *line, void *user_data)
{
    (void)user_data;
#if !CONFIG_INTERACTION_PEER_VOICE
    (void)line;
    return;
#else
    if (line == nullptr || line[0] == '\0') {
        return;
    }

    cJSON *root = cJSON_Parse(line);
    if (root == nullptr) {
        return;
    }

    const cJSON *type = cJSON_GetObjectItem(root, "t");
    const char *t = cJSON_IsString(type) ? type->valuestring : nullptr;

    if (t != nullptr && strcmp(t, "chat") == 0) {
        const cJSON *role = cJSON_GetObjectItem(root, "role");
        const cJSON *text = cJSON_GetObjectItem(root, "text");
        if (cJSON_IsString(text)) {
            peer_voice_ui_show_chat(cJSON_IsString(role) ? role->valuestring : "assistant",
                                    text->valuestring);
        }
    } else if (t != nullptr && strcmp(t, "emotion") == 0) {
        const cJSON *id = cJSON_GetObjectItem(root, "id");
        if (cJSON_IsString(id)) {
            peer_voice_ui_show_emotion(id->valuestring);
        }
    } else if (t != nullptr && strcmp(t, "voice_state") == 0) {
        const cJSON *state = cJSON_GetObjectItem(root, "state");
        if (cJSON_IsString(state)) {
            peer_voice_state_set(state->valuestring);
            peer_voice_ui_show_voice_state(state->valuestring);
        }
    }

    cJSON_Delete(root);
#endif
}

static void init_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(3000));
#if CONFIG_INTERACTION_PEER_VOICE
    peer_voice_ui_init();
    if (peer_uart_link_add_listener(on_voice_line, nullptr)) {
        ESP_LOGI(TAG, "voice RX listener registered");
    } else {
        ESP_LOGW(TAG, "voice RX listener registration failed");
    }
#endif
    vTaskDelete(nullptr);
}

void peer_voice_rx_init(void)
{
#if CONFIG_INTERACTION_PEER_VOICE
    xTaskCreatePinnedToCore(init_task, "peer_voice_rx", 3072, nullptr, 3, nullptr, 0);
#endif
}

#if CONFIG_INTERACTION_PEER_VOICE
static void peer_voice_rx_auto_init(void) __attribute__((constructor));
static void peer_voice_rx_auto_init(void)
{
    peer_voice_rx_init();
}
#endif
