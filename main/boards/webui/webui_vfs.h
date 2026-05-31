#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

bool webui_vfs_mount(void);
void webui_vfs_unmount(void);
bool webui_vfs_is_mounted(void);

/** Build full VFS path from web path like "/file.gcode". */
bool webui_vfs_resolve_path(const char *web_path, char *out, size_t out_size);

#ifdef __cplusplus
}
#endif
