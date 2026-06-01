#pragma once

/**
 * UART NDJSON protocol (interaction S3 -> motion S3), one JSON object per line.
 *
 * Host -> Peer commands:
 *   {"t":"ping"}
 *   {"t":"gcode","line":"G0 X1 Y1"}
 *   {"t":"jog","axis":"X","step":1.0,"sign":1}
 *   {"t":"home"}
 *   {"t":"move","x":10.0,"y":20.0}
 *   {"t":"pause"} / {"t":"run"}
 *   {"t":"apply","power":50,"speed":100}
 *   {"t":"file_begin","name":"job.gcode"}
 *   {"t":"file_end"}
 *   {"t":"poll"}
 *
 * Peer -> Host events:
 *   {"t":"pong","ok":true}
 *   {"t":"ack","ok":true}
 *   {"t":"status","state":0,"pct":0,"elapsed":0,"eta":0,"has_eta":false,"busy":false,"file":"-"}
 *   {"t":"pos","x":0.0,"y":0.0}
 *   {"t":"log","msg":"..."}
 *
 * Wiring: HOST TX(17) -> PEER RX, HOST RX(9) <- PEER TX, GND common.
 */

#include "../ui/cnc/ui_cnc_print_service.h"
#include "../ui/laser_ui_events.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool peer_cnc_client_init(void);
bool peer_cnc_client_is_ready(void);

bool peer_cnc_client_send_gcode(const char *line);
bool peer_cnc_client_jog_axis_mm(char axis, bool positive, float step_mm);
bool peer_cnc_client_home_async(void);
bool peer_cnc_client_move_to_mm_async(float x_mm, float y_mm);
bool peer_cnc_client_execute_gcode_file(const char *vfs_path);
bool peer_cnc_client_pause(void);
bool peer_cnc_client_run(void);
bool peer_cnc_client_apply_settings(void);

void peer_cnc_client_get_status(ui_cnc_print_status_t *out);
void peer_cnc_client_get_position_mm(float *x_mm, float *y_mm);
bool peer_cnc_client_is_busy(void);
const char *peer_cnc_client_get_job_name(void);
bool peer_cnc_client_has_suspended_job(void);

void peer_cnc_client_on_ui_event(laser_ui_event_id_t id);

#ifdef __cplusplus
}
#endif
