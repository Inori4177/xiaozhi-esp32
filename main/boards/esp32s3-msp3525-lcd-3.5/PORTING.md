# MSP3525 激光 UI 移植说明

## 复制到新工程时带上这些

```
esp32s3-msp3525-lcd-3.5/
  board_custom_ui.cc / .h    # 与小智核心的唯一对接实现
  ui/                        # 全部 LVGL 界面
  ft6336_touch.*             # 触摸（若硬件相同）
  esp32s3_msp3525_lcd_3_5.cc
  config.h / config.json
```

## 在小智 main 里只需保留的公共改动（一次性）

| 文件 | 作用 |
|------|------|
| `boards/common/board_custom_ui.h/.cc` | 通用钩子 API，`Application` / `LcdDisplay` 只调这里 |
| `boards/common/board.h` | `virtual GetCustomUiOps()`，默认 `nullptr` |
| `application.cc` | `BoardUiOn*` 调用，无 `laser_ui_*` 头文件 |
| `display/lcd_display.cc` | `BoardUiOnChatMessage`；`SetOverlayChromeVisible` 为通用顶栏显隐 |
| `CMakeLists.txt` | `board_custom_ui.cc` + `CONFIG_MSP3525_LASER_UI` 时编入 `ui/` |

其它板子不实现 `GetCustomUiOps()` 时行为与原版小智一致。

## 新板子接入步骤

1. 复制本目录，改 `BOARD_TYPE` / `config.json` 的 `sdkconfig_append`（`CONFIG_MSP3525_LASER_UI=y`）。
2. 在板级类中 override `GetCustomUiOps()`，返回你的 `BoardCustomUiOps`（可参考 `board_custom_ui.cc`）。
3. `BoardUiSplashStep` 枚举与 `ui/laser_ui_splash.h` 中 step 值保持一致。
4. `CMakeLists.txt` 中为该 `BOARD_TYPE` 增加 `ui/` 源文件 glob（照抄 MSP3525 段）。

## 业务逻辑

在板目录新增 `laser_controller.cc`，`laser_ui_events_register()` 处理雕刻事件，无需再改 `application.cc`。
