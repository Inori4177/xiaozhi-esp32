#pragma once

#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WEBUI_VFS_MOUNT     "/localfs"
#define WEBUI_VFS_LABEL     "storage"
#define WEBUI_WS_PATH         "/ws"
#define WEBUI_CMD_PATH        "/command"
#define WEBUI_FILES_PATH      "/files"
#define WEBUI_STATUS_PATH     "/status"
#define WEBUI_SETTINGS_PATH   "/settings"
#define WEBUI_PICK_PATH       "/pick"
#define WEBUI_RUN_PATH        "/run"
#define WEBUI_PAUSE_PATH      "/pause"
#define WEBUI_CHAT_PATH         "/chat"
#define WEBUI_PREVIEW_PATH      "/preview"

/** 图文 G-code 上传体积上限（字节），防止占满 1MB LocalFS / 内存。 */
#define WEBUI_GCODEGEN_MAX_BYTES  (120 * 1024)

#define WEBUI_LOG_RING_SIZE   4096
#define WEBUI_WS_CLIENT_MAX   4
#define WEBUI_CMD_QUEUE_LEN   16
#define WEBUI_CMD_LINE_MAX    256
#define WEBUI_FILE_PATH_MAX   128

#ifdef CONFIG_XIAOZHI_WEBUI_HTTP_PORT
#define WEBUI_HTTP_PORT CONFIG_XIAOZHI_WEBUI_HTTP_PORT
#else
#define WEBUI_HTTP_PORT 80
#endif

#ifdef __cplusplus
}
#endif
