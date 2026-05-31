#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void webui_service_start(void);
void webui_service_stop(void);
bool webui_service_is_running(void);

#ifdef __cplusplus
}
#endif
