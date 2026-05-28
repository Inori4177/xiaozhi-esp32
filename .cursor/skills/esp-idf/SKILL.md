---
name: esp-idf
description: >-
  ESP-IDF / 乐鑫嵌入式开发：构建配置、分区表、LVGL、FreeRTOS、板级驱动与调试。
  在涉及 ESP32、ESP-IDF、sdkconfig、CMake、乐鑫 API 或硬件引脚时使用。
---

# ESP-IDF / 乐鑫开发

## 文档与 MCP

遇到 ESP-IDF、乐鑫 API、Kconfig、驱动或芯片特性**不确定**时：

1. 优先使用已配置的 **user-espressif-docs** MCP（`search_espressif_sources`）。
2. 查询时根据用户语言选择 `language`: `cn`（中文）或 `en`（英文，默认）。
3. 不要凭记忆编造寄存器、API 签名或已废弃的配置项；以官方文档检索结果为准。

## 本项目约定

- 板型与引脚：以 `main/boards/<board>/config.h`、`config.json` 为准，修改前先读对应板级文件。
- 构建：ESP-IDF 标准流程（`idf.py set-target`、`idf.py build`）；分区表见 `partitions/`。
- UI：激光/CNC 相关在 `main/boards/bread-compact-wifi/`；CNC 逻辑在 `main/boards/CNC/`。
- 小智语音：`main/application.cc`；显示 `main/display/`。
- 变更范围：只改与任务相关的板级/模块，避免无关板型或全局重构。

## 常见任务检查清单

| 任务 | 注意点 |
|------|--------|
| 分区/Flash 不足 | 检查 `partitions/*.csv`、`sdkconfig` 中分区与 PSRAM |
| LCD/LVGL | 显示驱动在 `lcd_display.cc`；资源在板级 `ui/assets/` |
| Wi-Fi / 协议 | `main/protocols/`；MQTT/WebSocket 见 `docs/` |
| 新板级支持 | 参考 `docs/custom-board.md` 与现有 `main/boards/*` 结构 |

## 调试

- 串口日志：`idf.py monitor`；关注 boot 分区、堆栈、看门狗与任务栈水位。
- 配置漂移：对比 `sdkconfig.defaults` 与 `sdkconfig.defaults.esp32s3`。
