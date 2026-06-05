# Fresh Light LVGL UI Design

## Goal

Redesign `main/boards/bread-compact-wifi/ui` with a fresh, soft, light pastel style for the 480x320 landscape LVGL UI.

## Fixed Boundaries

- Keep the top status bar and bottom status bar structure.
- Keep the existing navigation buoy behavior and geometry.
- Keep the current jog control pad design, image assets, hit targets, and events.
- Keep all backend services and event IDs unchanged.

## Visual Direction

- Background: airy mint and cream, with a soft peach wash near the lower area.
- Cards: off-white rounded rectangles with pale mint borders, low-opacity shadows, and subtle outlines.
- Text: dark blue-gray for readability on light surfaces.
- Accents: mint/aqua for normal and active states, peach/coral for high-emphasis actions.
- LVGL fit: simple rectangles, circles, bars, sliders, labels, and existing image assets.

## Files

- `laser_ui_layout.h`: add light page theme tokens while preserving buoy geometry.
- `laser_ui_widgets.cc`: restyle shared panel, button, slider, status bar, map grid, and background helpers.
- `pages/page_print.cc`: restyle status and surrounding controls; do not replace jog pad assets.
- `pages/page_pick.cc`: restyle map card and side rail.
- `pages/page_settings.cc`: restyle settings panel, dropdown, sliders, and apply button.
- `assets/png` and `assets/images`: add optional pastel screen background asset for the page shell.

## Validation

- Static checks confirm protected jog pad assets remain referenced.
- Static checks confirm light theme tokens are present.
- Build with `idf.py build` when the ESP-IDF environment is available.
