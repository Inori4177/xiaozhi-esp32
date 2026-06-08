#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

enum class UiGcodeSegKind : uint8_t {
    Travel = 0,
    Engrave = 1,
};

struct UiGcodeSegment {
    float x0;
    float y0;
    float x1;
    float y1;
    UiGcodeSegKind kind;
};

/** Parse G-code text (G0/G1/M3/M5/G90/G91, XY mm). Returns false if no drawable segments. */
bool ui_gcode_preview_parse_text(const char *text, size_t len, std::vector<UiGcodeSegment> &segments_out);

/** Parse file from VFS path; size must be <= max_bytes. */
bool ui_gcode_preview_parse_file(const char *vfs_path, size_t max_bytes,
                                 std::vector<UiGcodeSegment> &segments_out);

/** Rasterize segments into RGB565 buffer (w×h). work_mm = physical work area side length. */
void ui_gcode_preview_rasterize(const std::vector<UiGcodeSegment> &segments, uint16_t *buf, int w,
                                int h, float work_mm, uint16_t bg_rgb565, uint16_t travel_rgb565,
                                uint16_t engrave_rgb565);
