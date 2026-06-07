#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void peer_voice_state_set(const char *state);
const char *peer_voice_state_get(void);

#ifdef __cplusplus
}
#endif
