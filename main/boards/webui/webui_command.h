#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 执行命令并将回复写入 resp（可广播到 WS）。返回 false 表示 CNC/FS 未就绪。 */
bool webui_command_dispatch(const char *cmd, char *resp, size_t resp_size);

void webui_command_dispatch_async(const char *cmd);

#ifdef __cplusplus
}
#endif
