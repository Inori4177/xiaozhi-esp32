#include "ui_gcode_preview.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

constexpr float kDefaultWorkMm = 42.0f;

void strip_inline_comments(char *line)
{
    if (line == nullptr) {
        return;
    }
    bool in_paren = false;
    for (char *p = line; *p != '\0'; ++p) {
        if (*p == '(') {
            in_paren = true;
            *p = ' ';
            continue;
        }
        if (*p == ')') {
            in_paren = false;
            *p = ' ';
            continue;
        }
        if (!in_paren && *p == ';') {
            *p = '\0';
            break;
        }
    }
}

void uppercase_inplace(char *s)
{
    if (s == nullptr) {
        return;
    }
    for (; *s != '\0'; ++s) {
        *s = static_cast<char>(std::toupper(static_cast<unsigned char>(*s)));
    }
}

bool token_is_g0(const char *tok)
{
    return tok != nullptr && (strcmp(tok, "G0") == 0 || strcmp(tok, "G00") == 0);
}

bool token_is_g1(const char *tok)
{
    return tok != nullptr && (strcmp(tok, "G1") == 0 || strcmp(tok, "G01") == 0);
}

bool parse_axis_value(const char *tok, char axis, float *out)
{
    if (tok == nullptr || out == nullptr || tok[0] != axis) {
        return false;
    }
    char *end = nullptr;
    const float v = std::strtof(tok + 1, &end);
    if (end == tok + 1) {
        return false;
    }
    *out = v;
    return true;
}

void draw_pixel(uint16_t *buf, int w, int h, int x, int y, uint16_t color)
{
    if (x < 0 || y < 0 || x >= w || y >= h) {
        return;
    }
    buf[static_cast<size_t>(y) * static_cast<size_t>(w) + static_cast<size_t>(x)] = color;
}

void draw_thick_pixel(uint16_t *buf, int w, int h, int x, int y, uint16_t color, int thickness)
{
    const int r = thickness > 1 ? 1 : 0;
    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            draw_pixel(buf, w, h, x + dx, y + dy, color);
        }
    }
}

void draw_line(uint16_t *buf, int w, int h, int x0, int y0, int x1, int y1, uint16_t color,
               int thickness)
{
    int dx = std::abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        draw_thick_pixel(buf, w, h, x0, y0, color, thickness);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

int mm_to_px_x(float mm, int w, float work_mm)
{
    if (work_mm <= 0.0f) {
        work_mm = kDefaultWorkMm;
    }
    const float t = mm / work_mm;
    return static_cast<int>(std::lround(t * static_cast<float>(w - 1)));
}

int mm_to_px_y(float mm, int h, float work_mm)
{
    if (work_mm <= 0.0f) {
        work_mm = kDefaultWorkMm;
    }
    const float t = 1.0f - (mm / work_mm);
    return static_cast<int>(std::lround(t * static_cast<float>(h - 1)));
}

void draw_border(uint16_t *buf, int w, int h, uint16_t color)
{
    for (int x = 0; x < w; ++x) {
        draw_pixel(buf, w, h, x, 0, color);
        draw_pixel(buf, w, h, x, h - 1, color);
    }
    for (int y = 0; y < h; ++y) {
        draw_pixel(buf, w, h, 0, y, color);
        draw_pixel(buf, w, h, w - 1, y, color);
    }
}

bool parse_line(const char *line_in, float &cur_x, float &cur_y, bool &abs_mode, bool &laser_on,
                std::vector<UiGcodeSegment> &segments_out)
{
    char line[256];
    if (line_in == nullptr) {
        return false;
    }
    strncpy(line, line_in, sizeof(line) - 1);
    line[sizeof(line) - 1] = '\0';
    strip_inline_comments(line);
    uppercase_inplace(line);

    char *start = line;
    while (*start != '\0' && std::isspace(static_cast<unsigned char>(*start))) {
        ++start;
    }
    if (*start == '\0') {
        return false;
    }

    bool is_g0 = false;
    bool is_g1 = false;
    bool has_x = false;
    bool has_y = false;
    float x_val = 0.0f;
    float y_val = 0.0f;

    const char *p = start;
    while (*p != '\0') {
        while (*p != '\0' && std::isspace(static_cast<unsigned char>(*p))) {
            ++p;
        }
        if (*p == '\0') {
            break;
        }
        const char *tok_start = p;
        while (*p != '\0' && !std::isspace(static_cast<unsigned char>(*p))) {
            ++p;
        }
        const char saved = *p;
        char *mutable_p = const_cast<char *>(p);
        if (saved != '\0') {
            *mutable_p = '\0';
            ++p;
        }
        char *tok = const_cast<char *>(tok_start);

        if (strcmp(tok, "G90") == 0) {
            abs_mode = true;
        } else if (strcmp(tok, "G91") == 0) {
            abs_mode = false;
        } else if (strcmp(tok, "M3") == 0 || strcmp(tok, "M03") == 0) {
            laser_on = true;
        } else if (strcmp(tok, "M5") == 0 || strcmp(tok, "M05") == 0) {
            laser_on = false;
        } else if (token_is_g0(tok)) {
            is_g0 = true;
        } else if (token_is_g1(tok)) {
            is_g1 = true;
        } else if (parse_axis_value(tok, 'X', &x_val)) {
            has_x = true;
        } else if (parse_axis_value(tok, 'Y', &y_val)) {
            has_y = true;
        }

        if (saved != '\0') {
            *mutable_p = saved;
        }
    }

    if (!is_g0 && !is_g1) {
        return false;
    }

    const float nx = has_x ? (abs_mode ? x_val : cur_x + x_val) : cur_x;
    const float ny = has_y ? (abs_mode ? y_val : cur_y + y_val) : cur_y;
    if (std::fabs(nx - cur_x) < 1e-6f && std::fabs(ny - cur_y) < 1e-6f) {
        return false;
    }

    UiGcodeSegment seg{};
    seg.x0 = cur_x;
    seg.y0 = cur_y;
    seg.x1 = nx;
    seg.y1 = ny;
    if (is_g1 && !is_g0 && laser_on) {
        seg.kind = UiGcodeSegKind::Engrave;
    } else {
        seg.kind = UiGcodeSegKind::Travel;
    }
    segments_out.push_back(seg);
    cur_x = nx;
    cur_y = ny;
    return true;
}

}  // namespace

bool ui_gcode_preview_parse_text(const char *text, size_t len, std::vector<UiGcodeSegment> &segments_out)
{
    segments_out.clear();
    if (text == nullptr || len == 0) {
        return false;
    }

    float cur_x = 0.0f;
    float cur_y = 0.0f;
    bool abs_mode = true;
    bool laser_on = false;

    std::string chunk(text, len);
    size_t pos = 0;
    while (pos <= chunk.size()) {
        const size_t nl = chunk.find('\n', pos);
        const std::string line = chunk.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos);
        if (!line.empty()) {
            parse_line(line.c_str(), cur_x, cur_y, abs_mode, laser_on, segments_out);
        }
        if (nl == std::string::npos) {
            break;
        }
        pos = nl + 1;
    }
    return !segments_out.empty();
}

bool ui_gcode_preview_parse_file(const char *vfs_path, size_t max_bytes,
                                 std::vector<UiGcodeSegment> &segments_out)
{
    segments_out.clear();
    if (vfs_path == nullptr || vfs_path[0] == '\0') {
        return false;
    }

    FILE *f = fopen(vfs_path, "rb");
    if (f == nullptr) {
        return false;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return false;
    }
    const long file_size = ftell(f);
    if (file_size < 0 || static_cast<size_t>(file_size) > max_bytes) {
        fclose(f);
        return false;
    }
    rewind(f);

    std::string content;
    content.resize(static_cast<size_t>(file_size));
    if (file_size > 0 &&
        fread(&content[0], 1, static_cast<size_t>(file_size), f) != static_cast<size_t>(file_size)) {
        fclose(f);
        return false;
    }
    fclose(f);
    return ui_gcode_preview_parse_text(content.data(), content.size(), segments_out);
}

void ui_gcode_preview_rasterize(const std::vector<UiGcodeSegment> &segments, uint16_t *buf, int w,
                                int h, float work_mm, uint16_t bg_rgb565, uint16_t travel_rgb565,
                                uint16_t engrave_rgb565)
{
    if (buf == nullptr || w <= 0 || h <= 0) {
        return;
    }
    const size_t pixels = static_cast<size_t>(w) * static_cast<size_t>(h);
    for (size_t i = 0; i < pixels; ++i) {
        buf[i] = bg_rgb565;
    }
    draw_border(buf, w, h, travel_rgb565);

    for (const auto &seg : segments) {
        const int x0 = mm_to_px_x(seg.x0, w, work_mm);
        const int y0 = mm_to_px_y(seg.y0, h, work_mm);
        const int x1 = mm_to_px_x(seg.x1, w, work_mm);
        const int y1 = mm_to_px_y(seg.y1, h, work_mm);
        const bool engrave = seg.kind == UiGcodeSegKind::Engrave;
        draw_line(buf, w, h, x0, y0, x1, y1, engrave ? engrave_rgb565 : travel_rgb565,
                  engrave ? 2 : 1);
    }
}
