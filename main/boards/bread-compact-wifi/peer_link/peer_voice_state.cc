#include "peer_voice_state.h"

#include <cstdio>
#include <cstring>

static char s_voice_state[16] = "idle";

void peer_voice_state_set(const char *state)
{
    if (state == nullptr || state[0] == '\0') {
        return;
    }
    snprintf(s_voice_state, sizeof(s_voice_state), "%s", state);
}

const char *peer_voice_state_get(void)
{
    return s_voice_state;
}
