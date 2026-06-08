#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void ui_pick_keypad_format_initial(float value_mm, char *buf, size_t buf_size)
{
    if (buf == NULL || buf_size == 0) {
        return;
    }
    snprintf(buf, buf_size, "%.1f", (double)value_mm);
}

static inline bool ui_pick_keypad_apply_token(char *buf, size_t buf_size, char token)
{
    if (buf == NULL || buf_size == 0) {
        return false;
    }

    const size_t len = strlen(buf);
    if (token == '\b') {
        if (len == 0) {
            return false;
        }
        buf[len - 1] = '\0';
        return true;
    }

    if (token == '.') {
        if (len == 0 || strchr(buf, '.') != NULL || len >= 4) {
            return false;
        }
        buf[len] = '.';
        buf[len + 1] = '\0';
        return true;
    }

    if (token < '0' || token > '9' || len >= 4) {
        return false;
    }

    if (strcmp(buf, "0.0") == 0 || strcmp(buf, "0") == 0) {
        buf[0] = token;
        buf[1] = '\0';
        return true;
    }

    const char *dot = strchr(buf, '.');
    if (dot != NULL && dot[1] != '\0') {
        return false;
    }

    buf[len] = token;
    buf[len + 1] = '\0';
    return true;
}

static inline bool ui_pick_keypad_commit_value(const char *buf, float min_mm, float max_mm, float *out_value_mm)
{
    if (buf == NULL || out_value_mm == NULL || buf[0] == '\0' || strcmp(buf, ".") == 0) {
        return false;
    }

    char *end = NULL;
    float parsed = strtof(buf, &end);
    if (end == buf || (end != NULL && *end != '\0')) {
        return false;
    }

    if (parsed < min_mm) {
        parsed = min_mm;
    } else if (parsed > max_mm) {
        parsed = max_mm;
    }

    *out_value_mm = parsed;
    return true;
}

#ifdef __cplusplus
}
#endif
