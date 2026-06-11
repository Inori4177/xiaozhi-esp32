#pragma once

#include "../cnc/ui_cnc_coord_map.h"

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
    /** Index among peer-sendable file lines (see parse_file_for_send); UINT32_MAX if unknown. */
    uint32_t source_line_idx;
};

/** Parse G-code text (G0/G1/M3/M5/G90/G91, XY mm). Returns false if no drawable segments. */
bool ui_gcode_preview_parse_text(const char *text, size_t len, std::vector<UiGcodeSegment> &segments_out);

/** Parse file from VFS path; size must be <= max_bytes. */
bool ui_gcode_preview_parse_file(const char *vfs_path, size_t max_bytes,
                                 std::vector<UiGcodeSegment> &segments_out);

/**
 * Parse like peer CNC file send: same line filtering, tag each segment with source_line_idx.
 * total_send_lines_out receives the line count used for progress_pct (optional).
 */
bool ui_gcode_preview_parse_file_for_send(const char *vfs_path, size_t max_bytes,
                                          std::vector<UiGcodeSegment> &segments_out,
                                          uint32_t *total_send_lines_out);

/** Shift all segment endpoints by pick-origin offset (machine mm). */
void ui_gcode_preview_offset_segments(std::vector<UiGcodeSegment> &segments, float ox_mm,
                                      float oy_mm);

/**
 * Apply pick-origin offset to G0/G1 absolute XY on one line; honors G90/G91 via abs_mode in/out.
 * Returns true if line buffer was rewritten.
 */
bool ui_gcode_preview_offset_gcode_line(char *line, size_t line_cap, bool *abs_mode, float ox_mm,
                                        float oy_mm);

/** Rasterize segments into RGB565 buffer (w×h). work_mm = physical work area side length. */
void ui_gcode_preview_rasterize(const std::vector<UiGcodeSegment> &segments, uint16_t *buf, int w,
                                int h, float work_mm, uint16_t bg_rgb565, uint16_t travel_rgb565,
                                uint16_t engrave_rgb565);

/** Sentinel pixel value — transparent / skip when used as pick-map overlay chroma key. */
#define UI_GCODE_PREVIEW_CHROMA_KEY 0x0001U

/**
 * Rasterize segments with per-segment colors into a canvas buffer aligned to viewport.
 * Skips Travel segments. Fills buffer with UI_GCODE_PREVIEW_CHROMA_KEY first.
 * seg_colors.size() must equal segments.size(); nullptr vp uses full canvas as 42 mm area.
 */
void ui_gcode_preview_rasterize_colored(const std::vector<UiGcodeSegment> &segments,
                                        const std::vector<uint16_t> &seg_colors, uint16_t *buf,
                                        int w, int h, const ui_cnc_coord_viewport_t *vp);

/** ARGB8888 buffer (w×h uint32_t), transparent background; skips Travel segments. */
void ui_gcode_preview_rasterize_colored_argb(const std::vector<UiGcodeSegment> &segments,
                                             const std::vector<uint32_t> &seg_colors_argb,
                                             uint32_t *buf, int w, int h,
                                             const ui_cnc_coord_viewport_t *vp);

/**
 * Paint engrave segments green, cover done portion black by file send progress (0–100).
 * Uses segment.source_line_idx vs total_send_lines (peer file line order, M3/G0 included).
 */
void ui_gcode_preview_rasterize_progress_argb(const std::vector<UiGcodeSegment> &segments,
                                              uint32_t pending_argb, uint32_t done_argb,
                                              uint8_t progress_pct, uint32_t total_send_lines,
                                              uint32_t *buf, int w, int h,
                                              const ui_cnc_coord_viewport_t *vp);
