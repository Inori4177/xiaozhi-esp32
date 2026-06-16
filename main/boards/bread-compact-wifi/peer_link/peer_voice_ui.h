#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void peer_voice_ui_init(void);
void peer_voice_ui_show_chat(const char *role, const char *text);
void peer_voice_ui_show_emotion(const char *emotion_id);
void peer_voice_ui_show_voice_state(const char *state);
const char *peer_voice_ui_get_last_chat_role(void);
const char *peer_voice_ui_get_last_chat_text(void);
int peer_voice_ui_get_chat_count(void);
const char *peer_voice_ui_get_chat_role(int index);
const char *peer_voice_ui_get_chat_text(int index);

#ifdef __cplusplus
}
#endif
