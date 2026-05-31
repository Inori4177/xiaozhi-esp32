#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 执行命令并将回复写入 resp。broadcast_ws=false 时仅写日志，由 WS handler 单独回包。 */
bool webui_command_dispatch(const char *cmd, char *resp, size_t resp_size);

/** WS 专用：不广播，避免与单帧回包重复。 */
bool webui_command_dispatch_ws(const char *cmd, char *resp, size_t resp_size);

void webui_command_dispatch_async(const char *cmd);

#ifdef __cplusplus
}
#endif
