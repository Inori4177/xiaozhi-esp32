#include "ui_gcode_preview.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

static int g_failures = 0;

static void expect_true(bool cond, const char *msg)
{
    if (!cond) {
        std::fprintf(stderr, "FAIL: %s\n", msg);
        ++g_failures;
    }
}

static void expect_seg(const UiGcodeSegment &seg, float x0, float y0, float x1, float y1,
                       UiGcodeSegKind kind, const char *msg)
{
    if (std::fabs(seg.x0 - x0) > 0.001f || std::fabs(seg.y0 - y0) > 0.001f ||
        std::fabs(seg.x1 - x1) > 0.001f || std::fabs(seg.y1 - y1) > 0.001f ||
        seg.kind != kind) {
        std::fprintf(stderr,
                     "FAIL: %s (got %.3f,%.3f->%.3f,%.3f kind=%d)\n", msg, static_cast<double>(seg.x0),
                     static_cast<double>(seg.y0), static_cast<double>(seg.x1), static_cast<double>(seg.y1),
                     static_cast<int>(seg.kind));
        ++g_failures;
    }
}

int main()
{
    std::vector<UiGcodeSegment> segs;
    const char *sample =
        "G90\n"
        "G0 X0 Y0\n"
        "M3 S500\n"
        "G1 X10 Y0\n"
        "M5\n"
        "G0 X0 Y10\n";

    expect_true(ui_gcode_preview_parse_text(sample, strlen(sample), segs), "parses sample gcode");
    expect_true(segs.size() >= 2, "sample has multiple segments");
    bool found_engrave = false;
    bool found_travel = false;
    for (const auto &s : segs) {
        if (s.kind == UiGcodeSegKind::Engrave && std::fabs(s.x1 - 10.0f) < 0.001f) {
            found_engrave = true;
        }
        if (s.kind == UiGcodeSegKind::Travel && std::fabs(s.y1 - 10.0f) < 0.001f) {
            found_travel = true;
        }
    }
    expect_true(found_engrave, "finds M3+G1 engrave segment");
    expect_true(found_travel, "finds post-M5 travel segment");

    segs.clear();
    const char *rel =
        "G91\n"
        "G0 X5 Y0\n"
        "G1 X5 Y0\n";
    expect_true(ui_gcode_preview_parse_text(rel, strlen(rel), segs), "parses relative gcode");
    expect_true(!segs.empty(), "relative produces segments");
    expect_seg(segs[0], 0, 0, 5, 0, UiGcodeSegKind::Travel, "G91 G0 relative X");

    segs.clear();
    expect_true(!ui_gcode_preview_parse_text("; only comment\n", 16, segs), "empty comments fail");
    expect_true(segs.empty(), "no segments for comments only");

    if (g_failures == 0) {
        std::fprintf(stdout, "All gcode preview tests passed.\n");
        return 0;
    }
    std::fprintf(stderr, "%d test(s) failed.\n", g_failures);
    return 1;
}
