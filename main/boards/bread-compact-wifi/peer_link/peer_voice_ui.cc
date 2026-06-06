#include "peer_voice_ui.h"

#include "board.h"
#include "display.h"

#include <cstring>

#include <esp_log.h>

static const char *TAG = "peer_voice_ui";

void peer_voice_ui_init(void)
{
    ESP_LOGI(TAG, "voice UI bridge ready");
}

void peer_voice_ui_show_chat(const char *role, const char *text)
{
    if (text == nullptr || text[0] == '\0') {
        return;
    }
    auto *display = Board::GetInstance().GetDisplay();
    if (display == nullptr) {
        return;
    }
    display->SetChatMessage(role != nullptr ? role : "assistant", text);
    ESP_LOGI(TAG, "chat [%s]: %.48s", role != nullptr ? role : "?", text);
}

void peer_voice_ui_show_emotion(const char *emotion_id)
{
    if (emotion_id == nullptr || emotion_id[0] == '\0') {
        return;
    }
    auto *display = Board::GetInstance().GetDisplay();
    if (display == nullptr) {
        return;
    }
    display->SetEmotion(emotion_id);
}

void peer_voice_ui_show_voice_state(const char *state)
{
    if (state == nullptr) {
        return;
    }
    auto *display = Board::GetInstance().GetDisplay();
    if (display == nullptr) {
        return;
    }

    char msg[64];
    snprintf(msg, sizeof(msg), "Voice: %s", state);
    display->ShowNotification(msg, 2000);
}
