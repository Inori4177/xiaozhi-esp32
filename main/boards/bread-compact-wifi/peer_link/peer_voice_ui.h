#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void peer_voice_ui_init(void);
void peer_voice_ui_show_chat(const char *role, const char *text);
void peer_voice_ui_show_emotion(const char *emotion_id);
void peer_voice_ui_show_voice_state(const char *state);

#ifdef __cplusplus
}
#endif
