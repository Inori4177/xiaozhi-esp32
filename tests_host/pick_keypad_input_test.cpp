#include "main/boards/bread-compact-wifi/ui/pick/ui_pick_keypad_input.h"

#include <cmath>
#include <cstdio>
#include <cstring>

static int g_failures = 0;

static void expect_true(bool cond, const char *msg)
{
    if (!cond) {
        std::fprintf(stderr, "FAIL: %s\n", msg);
        ++g_failures;
    }
}

static void expect_str_eq(const char *actual, const char *expected, const char *msg)
{
    if (std::strcmp(actual, expected) != 0) {
        std::fprintf(stderr, "FAIL: %s (actual='%s' expected='%s')\n", msg, actual, expected);
        ++g_failures;
    }
}

static void expect_float_eq(float actual, float expected, const char *msg)
{
    if (std::fabs(actual - expected) > 0.0001f) {
        std::fprintf(stderr, "FAIL: %s (actual=%.4f expected=%.4f)\n", msg, static_cast<double>(actual),
                     static_cast<double>(expected));
        ++g_failures;
    }
}

int main()
{
    char buf[16];

    ui_pick_keypad_format_initial(12.3f, buf, sizeof(buf));
    expect_str_eq(buf, "12.3", "formats current mm with one decimal place");

    ui_pick_keypad_format_initial(0.0f, buf, sizeof(buf));
    expect_str_eq(buf, "0.0", "formats zero with one decimal place");

    std::strcpy(buf, "0.0");
    expect_true(ui_pick_keypad_apply_token(buf, sizeof(buf), '4'), "accepts replacement digit");
    expect_str_eq(buf, "4", "digit replaces default zero text");

    expect_true(ui_pick_keypad_apply_token(buf, sizeof(buf), '2'), "accepts second digit");
    expect_str_eq(buf, "42", "second digit appends");

    expect_true(ui_pick_keypad_apply_token(buf, sizeof(buf), '.'), "accepts one decimal point");
    expect_str_eq(buf, "42.", "decimal point appends once");

    expect_true(ui_pick_keypad_apply_token(buf, sizeof(buf), '5'), "accepts first fractional digit");
    expect_str_eq(buf, "42.5", "fractional digit appends");

    expect_true(!ui_pick_keypad_apply_token(buf, sizeof(buf), '6'), "rejects extra fractional digit");
    expect_str_eq(buf, "42.5", "buffer stays unchanged when rejecting extra fractional digit");

    expect_true(!ui_pick_keypad_apply_token(buf, sizeof(buf), '.'), "rejects second decimal point");
    expect_str_eq(buf, "42.5", "buffer stays unchanged when rejecting second decimal point");

    expect_true(ui_pick_keypad_apply_token(buf, sizeof(buf), '\b'), "backspace removes last character");
    expect_str_eq(buf, "42.", "backspace trims fractional digit");

    expect_true(ui_pick_keypad_apply_token(buf, sizeof(buf), '\b'), "backspace removes decimal point");
    expect_str_eq(buf, "42", "backspace trims decimal point");

    float parsed_mm = 0.0f;
    expect_true(ui_pick_keypad_commit_value("42", 0.0f, 42.0f, &parsed_mm), "commits integer text");
    expect_float_eq(parsed_mm, 42.0f, "integer text commits to float");

    expect_true(ui_pick_keypad_commit_value("42.5", 0.0f, 42.0f, &parsed_mm), "commit clamps to max range");
    expect_float_eq(parsed_mm, 42.0f, "value above max clamps to 42.0");

    expect_true(ui_pick_keypad_commit_value(".5", 0.0f, 42.0f, &parsed_mm), "leading dot is accepted");
    expect_float_eq(parsed_mm, 0.5f, "leading dot parses to fraction");

    expect_true(!ui_pick_keypad_commit_value(".", 0.0f, 42.0f, &parsed_mm), "bare decimal point is invalid");

    if (g_failures != 0) {
        std::fprintf(stderr, "%d assertion(s) failed\n", g_failures);
        return 1;
    }

    std::puts("pick keypad input tests passed");
    return 0;
}
