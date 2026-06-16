#include "peer_voice_ui.h"

#include "board.h"
#include "display.h"

#include <cstring>
#include <cstdio>

#include <esp_log.h>

static const char *TAG = "peer_voice_ui";
static char s_last_chat_role[16] = "assistant";
static char s_last_chat_text[256] = "";
static constexpr int kMaxChatHistory = 8;
static char s_chat_roles[kMaxChatHistory][16] = {};
static char s_chat_texts[kMaxChatHistory][256] = {};
static int s_chat_count = 0;

static void clear_chat_history(void)
{
    s_last_chat_role[0] = '\0';
    s_last_chat_text[0] = '\0';
    s_chat_count = 0;
    memset(s_chat_roles, 0, sizeof(s_chat_roles));
    memset(s_chat_texts, 0, sizeof(s_chat_texts));
}

static void append_chat_history(const char *role, const char *text)
{
    if (text == nullptr || text[0] == '\0') {
        return;
    }
    int dst = s_chat_count;
    if (s_chat_count < kMaxChatHistory) {
        ++s_chat_count;
    } else {
        for (int i = 1; i < s_chat_count; ++i) {
            snprintf(s_chat_roles[i - 1], sizeof(s_chat_roles[i - 1]), "%s", s_chat_roles[i]);
            snprintf(s_chat_texts[i - 1], sizeof(s_chat_texts[i - 1]), "%s", s_chat_texts[i]);
        }
        dst = s_chat_count - 1;
    }
    snprintf(s_chat_roles[dst], sizeof(s_chat_roles[dst]), "%s", role != nullptr ? role : "assistant");
    snprintf(s_chat_texts[dst], sizeof(s_chat_texts[dst]), "%s", text);
}

void peer_voice_ui_init(void)
{
    ESP_LOGI(TAG, "voice UI bridge ready");
}

void peer_voice_ui_show_chat(const char *role, const char *text)
{
    if (text == nullptr || text[0] == '\0') {
        return;
    }
    snprintf(s_last_chat_role, sizeof(s_last_chat_role), "%s", role != nullptr ? role : "assistant");
    snprintf(s_last_chat_text, sizeof(s_last_chat_text), "%s", text);
    append_chat_history(role, text);
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

    if (strcmp(state, "idle") == 0 || strcmp(state, "standby") == 0) {
        clear_chat_history();
        display->ClearChatMessages();
        return;
    }

    char msg[64];
    snprintf(msg, sizeof(msg), "Voice: %s", state);
    display->ShowNotification(msg, 2000);
}

const char *peer_voice_ui_get_last_chat_role(void)
{
    return s_last_chat_role;
}

const char *peer_voice_ui_get_last_chat_text(void)
{
    return s_last_chat_text;
}

int peer_voice_ui_get_chat_count(void)
{
    return s_chat_count;
}

const char *peer_voice_ui_get_chat_role(int index)
{
    if (index < 0 || index >= s_chat_count) {
        return "";
    }
    return s_chat_roles[index];
}

const char *peer_voice_ui_get_chat_text(int index)
{
    if (index < 0 || index >= s_chat_count) {
        return "";
    }
    return s_chat_texts[index];
}
