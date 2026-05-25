#pragma once

#include <stddef.h>

/** Reference feed rate (mm/min) when motion speed UI shows 100%. */
#define LASER_GCODE_FEED_BASE_MM_MIN 1000

void laser_gcode_init(void);
void laser_gcode_send(const char *line);
void laser_gcode_sendf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
