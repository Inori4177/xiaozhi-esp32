# Fresh Light LVGL UI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Apply a fresh light pastel UI style to the bread compact WiFi LVGL pages without changing navigation buoy or jog pad behavior/design.

**Architecture:** Keep the existing LVGL C++ page structure and event wiring. Add light theme tokens and restyle shared widgets so pages inherit the new aesthetic, then apply small page-level color/layout refinements where needed.

**Tech Stack:** LVGL C/C++ in `main/boards/bread-compact-wifi/ui`, existing PNG-to-C asset pipeline, ESP-IDF build.

---

### Task 1: Guard Rails And Red Check

**Files:**
- Inspect: `main/boards/bread-compact-wifi/ui/laser_ui_layout.h`
- Inspect: `main/boards/bread-compact-wifi/ui/pages/page_print.cc`

- [ ] Confirm `UI_BUOY_COLLAPSED_X` is preserved as the current workspace value.
- [ ] Confirm `page_print.cc` still references `btn_pad_top`, `arrow_up`, `arrow_down`, `arrow_left`, and `arrow_right`.
- [ ] Run a static red check for the new light token `UI_COLOR_PAGE_BG_TOP`; expected before implementation: no match.

### Task 2: Light Theme Tokens

**Files:**
- Modify: `main/boards/bread-compact-wifi/ui/laser_ui_layout.h`

- [ ] Add page-specific light tokens for background, cards, text, borders, mint/aqua, peach/coral, and status colors.
- [ ] Preserve buoy geometry constants.
- [ ] Preserve buoy color tokens enough that navigation keeps its current visual identity.

### Task 3: Shared Widget Styling

**Files:**
- Modify: `main/boards/bread-compact-wifi/ui/laser_ui_widgets.cc`

- [ ] Restyle panels to off-white cards with pale borders and soft shadows.
- [ ] Restyle buttons to rounded pastel pills with dark text.
- [ ] Restyle sliders and bars to light tracks with mint/coral indicators.
- [ ] Restyle map grid to soft mint lines.
- [ ] Replace dark scanline/background helpers with soft decorative washes.

### Task 4: Page Refinements

**Files:**
- Modify: `main/boards/bread-compact-wifi/ui/pages/page_print.cc`
- Modify: `main/boards/bread-compact-wifi/ui/pages/page_pick.cc`
- Modify: `main/boards/bread-compact-wifi/ui/pages/page_settings.cc`

- [ ] Print page: restyle status panel and coordinate/step/control surfaces while leaving jog pad image assets unchanged.
- [ ] Pick page: restyle map frame, cursor, coordinate labels, confirm/reset buttons.
- [ ] Settings page: restyle dropdown, panel, sliders, and apply button.

### Task 5: Pastel Asset

**Files:**
- Create: `main/boards/bread-compact-wifi/ui/assets/png/fresh_light_background.png`
- Create: `main/boards/bread-compact-wifi/ui/assets/images/fresh_light_background.c`
- Modify if required: `main/boards/bread-compact-wifi/ui/assets/laser_ui_images.h`

- [ ] Add a 480x320 pastel background image for future shell/page usage.
- [ ] Keep existing jog pad and arrow assets unchanged.

### Task 6: Verification

**Files:**
- Verify: source files above

- [ ] Run static checks for protected references and light tokens.
- [ ] Run `idf.py build` if the ESP-IDF environment is available in this shell.
- [ ] Report exact verification evidence and any build environment limitation.
