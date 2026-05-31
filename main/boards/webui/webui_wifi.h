#pragma once

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Handle [ESP410]/[ESP401]/[ESP420] style commands; writes response into out. */
bool webui_wifi_handle_esp_command(const char *cmd, char *out, size_t out_size);

#ifdef __cplusplus
}
#endif
