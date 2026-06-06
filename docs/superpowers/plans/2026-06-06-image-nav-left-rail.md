# Image Nav Left Rail Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the left buoy navigation in the bread compact WiFi laser UI with a 4-state image navigation rail, including a blank Voice AI page placeholder.

**Architecture:** Keep the existing shell/page-switching structure, but replace the buoy object set with one image object that swaps among four state assets plus four transparent touch hotspots. Extend the page enum and page array to include a blank Voice AI page so the navigation can switch to a real fourth page without adding behavior yet.

**Tech Stack:** LVGL C/C++ UI code in `main/boards/bread-compact-wifi/ui`, generated LVGL image assets in `ui/assets/images`, ESP-IDF build.

---

### Task 1: Navigation Resource Wiring

**Files:**
- Modify: `main/boards/bread-compact-wifi/ui/assets/laser_ui_images.h`
- Create: `main/boards/bread-compact-wifi/ui/assets/images/printpage.c`
- Create: `main/boards/bread-compact-wifi/ui/assets/images/settingpage.c`
- Create: `main/boards/bread-compact-wifi/ui/assets/images/pickpage.c`
- Create: `main/boards/bread-compact-wifi/ui/assets/images/xiaozhipage.c`

- [ ] Add LVGL declarations for the four left-rail state images.
- [ ] Generate the corresponding `.c` image sources from the updated PNG assets.

### Task 2: Shell Geometry And Page Model

**Files:**
- Modify: `main/boards/bread-compact-wifi/ui/laser_ui_layout.h`
- Modify: `main/boards/bread-compact-wifi/ui/laser_ui_shell.h`

- [ ] Add constants for the image nav rail size, placement, and hotspot geometry using the approved `52x205` asset size and centered left-rail placement.
- [ ] Extend `LaserPage` and shell-owned page/nav arrays from 3 items to 4 items.

### Task 3: Voice AI Placeholder Page

**Files:**
- Create: `main/boards/bread-compact-wifi/ui/pages/page_voice_ai.h`
- Create: `main/boards/bread-compact-wifi/ui/pages/page_voice_ai.cc`

- [ ] Add a fourth page that renders an empty transparent background only.
- [ ] Keep the page free of controls so later features can be added without undoing this navigation work.

### Task 4: Shell Navigation Rewrite

**Files:**
- Modify: `main/boards/bread-compact-wifi/ui/laser_ui_shell.cc`

- [ ] Remove buoy-only creation and interaction from the shell implementation.
- [ ] Create one nav image object positioned at the approved left-rail coordinates.
- [ ] Create four transparent hotspots aligned to the four button centers.
- [ ] Switch the nav image asset when the current page changes.
- [ ] Keep page open/close animation behavior for content pages.

### Task 5: Verification

**Files:**
- Verify: files above

- [ ] Run a static search that confirms the new image assets and Voice AI page are referenced.
- [ ] Run an ESP-IDF build if the local shell has the environment available.
- [ ] Report exact verification evidence and any remaining limitation.
