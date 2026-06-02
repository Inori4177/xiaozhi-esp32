---
name: dual-s3-link
description: >-
  ESP32-S3 双机串口通信架构：交互 S3（屏+触摸+WebUI）↔ 运动/语音 S3（CNC+小智语音）。
  涉及 UART NDJSON 协议、peer_link、引脚分配、cnc_bridge、双工程（主工程 + xiaozhi-esp32-jdil）。
  触发词：双机通信、串口、UART、peer_link、双S3、CNC 交互、两块 ESP32、jdil。
---

# 双 ESP32-S3 串口通信架构

## 项目组成

```
xiaozhi-esp32-main/                  ← 交互 S3（主工程，本仓库）
├── main/boards/bread-compact-wifi/  ← 交互 S3 板级
│   ├── config.h                     ← 屏 SPI、触摸 I2C、UART 引脚
│   └── peer_link/                   ← 串口通信层（peer_uart_link + peer_cnc_client）
└── xiaozhi-esp32-jdil/              ← 运动/语音 S3（子工程）
    └── xiaozhi-esp32-main/
        └── main/boards/
            ├── bread-compact-wifi/   ← 运动 S3 板级（SSD1306 + I2S + CNC）
            └── CNC/                 ← 本地步进/激光控制（stepping_engine, stepper, motion_controller）
```

两个工程**独立编译**，各跑一块 ESP32-S3，通过 UART 交叉连接通信。

## 架构总览

```
手机浏览器 ──HTTP/WS──► 交互 S3 (主工程, InteractionApp)
                           │
    ┌──────────────────────┼──────────────────────┐
    │  LVGL 触摸 UI        │  WebUI (HTTP+WS)     │
    │  ST7796 480×320     │  webui_cnc_bridge    │
    │  FT6336 触摸         │  webui_command       │
    └──────────────────────┼──────────────────────┘
                           │ peer_cnc_client (NDJSON)
                           │ UART TX=17 ─────────────► UART RX=9
                           │ UART RX=9  ◄───────────── UART TX=17
                           │                           │
                    运动/语音 S3 (jdil 工程, Application) │
                           │
    ┌──────────────────────┼──────────────────────┐
    │  I2S 音频 (MIC+喇叭)  │  CNC 步进/激光控制     │
    │  小智语音助手         │  MotionController    │
    │  MQTT/WS → 云端      │  Stepper ISR         │
    └──────────────────────┴──────────────────────┘
```

## 引脚分配

### 交互 S3 (`main/boards/bread-compact-wifi/config.h`)

| 功能 | 引脚 |
|------|------|
| LCD SPI MOSI/MISO/SCK/CS/DC/RST/BL | 38/3/14/1/2/21/8 |
| 触摸 I2C SDA/SCL | 47/48 |
| 触摸 RST/INT | 41/42 |
| Boot/Vol+/Vol-/Touch/LED | 0/40/39/19/20 |
| **UART1 TX → 运动 S3 RX** | **17** |
| **UART1 RX ← 运动 S3 TX** | **18** |

### 运动/语音 S3 (`xiaozhi-esp32-jdil/.../boards/bread-compact-wifi/config.h`)

| 功能 | 引脚 |
|------|------|
| I2S MIC WS/SCK/DIN | 4/5/6 |
| I2S SPK BCLK/LRCK/DOUT | 15/16/7 |
| OLED I2C SDA/SCL | 3/8 |
| Boot/Touch/Vol+/Vol- | 0/47/40/39 |
| LED | 48 |

### 运动/语音 S3 CNC 引脚 (`xiaozhi-esp32-jdil/.../boards/CNC/stepping_engine.h`)

| 功能 | 当前引脚 | **冲突** | 建议改后 |
|------|---------|----------|---------|
| X STEP / DIR | 13 / 12 | — | 不变 |
| Y STEP / DIR | 10 / 11 | — | 不变 |
| **STEP ENABLE** | **9** | ← UART RX 冲突 | **14** |
| **LASER PWM** | **17** | ← UART TX 冲突 | **21** |

> jdil 侧必须改 `STEP_ENABLE_PIN` 和 `LASER_PWM_PIN` 两个宏，释放 GPIO9/17 给 UART。

## UART NDJSON 协议

波特率由 Kconfig `CONFIG_INTERACTION_PEER_UART_BAUD` 控制。每行一个 JSON 对象，以 `\n` 结尾。

### 现有协议（CNC 命令/状态）

**交互 S3 → 运动 S3:**

```json
{"t":"ping"}
{"t":"gcode","line":"G0 X1 Y1"}
{"t":"jog","axis":"X","step":1.0,"sign":1}
{"t":"home"}
{"t":"move","x":10.0,"y":20.0}
{"t":"pause"} / {"t":"run"}
{"t":"apply","power":50,"speed":100}
{"t":"file_begin","name":"job.gcode"}
{"t":"file_end"}
{"t":"poll"}
```

**运动 S3 → 交互 S3:**

```json
{"t":"pong","ok":true}
{"t":"ack","ok":true}
{"t":"status","state":0,"pct":0,"elapsed":0,"eta":0,"has_eta":false,"busy":false,"file":"-"}
{"t":"pos","x":0.0,"y":0.0}
{"t":"log","msg":"..."}
```

### 需新增协议（语音对话同步）

运动 S3 在执行语音对话时，需通过 UART 把对话内容推送给交互 S3 显示：

```json
{"t":"chat","role":"user","text":"今天天气怎么样"}
{"t":"chat","role":"assistant","text":"今天晴天，25度"}
{"t":"emotion","id":"happy"}
{"t":"voice_state","state":"listening"}
```

`voice_state` 取值：`idle` / `connecting` / `listening` / `speaking`。

## 关键代码文件

### 交互 S3 侧（主工程，已有但须扩展 chat 解析）

| 文件 | 职责 |
|------|------|
| `main/boards/bread-compact-wifi/peer_link/peer_uart_link.h` | UART 初始化、行收发、线程安全 |
| `main/boards/bread-compact-wifi/peer_link/peer_cnc_client.h` | CNC 语义封装（gcode/jog/home/pause…） |
| `main/boards/bread-compact-wifi/peer_link/peer_cnc_client.cc` | **需扩展**：`on_peer_line()` 新增 chat/emotion/voice_state 解析 |
| `main/boards/webui/webui_cnc_bridge.cc` | WebUI → CNC 桥接（`webui_cnc_submit_line` 等） |
| `main/boards/webui/webui_command.cc` | G-code 兼容命令处理 |
| `main/app_runtime.h` | `CONFIG_INTERACTION_UI_ONLY` 控制使用 `InteractionApp` |
| `main/interaction_app.cc` | 纯交互 App（无音频/语音） |

### 运动/语音 S3 侧（jdil 工程，需全部新建通信层）

| 文件 | 职责 |
|------|------|
| `main/boards/CNC/stepping_engine.h` | **须改**：`STEP_ENABLE` 和 `LASER_PWM` 引脚 |
| `main/boards/CNC/motion_controller.h` | 本地 G-code 执行引擎 |
| `main/boards/CNC/stepper.h` | Bresenham 步进 ISR |
| `main/boards/CNC/gcode_controller.h` | 激光雕刻 MCP 工具（`KanjiVGController`） |
| `main/application.cc` | **须改**：`OnIncomingJson` 中追加 UART 推送 chat/emotion |
| `main/device_state_machine.cc` | **须改**：`HandleStateChanged` 中追加 voice_state 推送 |

### jdil 侧需新建的文件

```
xiaozhi-esp32-jdil/xiaozhi-esp32-main/main/boards/bread-compact-wifi/
└── peer_link/                    ← 新建目录
    ├── peer_uart_link.h          ← 移植自主工程或重写
    ├── peer_uart_link.cc         ← UART 收发 + FreeRTOS 任务
    ├── peer_cnc_slave.h          ← 新建：处理 HOST 命令 + 推送状态
    └── peer_cnc_slave.cc         ← 新建：ping/gcode/jog 处理 + status/pos 推送
```

## 集成实施步骤

### 第 1 步：jdil 改引脚（小改动）

编辑 `xiaozhi-esp32-jdil/.../boards/CNC/stepping_engine.h`：

```cpp
#define STEP_ENABLE_PIN       GPIO_NUM_14   // 原 9
#define LASER_PWM_PIN         GPIO_NUM_21   // 原 17
```

### 第 2 步：jdil 新建 peer_uart_link

移植 `peer_uart_link.cc/h`（参考主工程的 `peer_link/peer_uart_link.cc`）。关键配置：

```cpp
#define PEER_UART_NUM      UART_NUM_1
#define PEER_UART_TX_PIN   GPIO_NUM_17     // jdil TX → 交互 S3 RX
#define PEER_UART_RX_PIN   GPIO_NUM_9      // jdil RX ← 交互 S3 TX
```

核心：`rx_task` 逐字读 UART，按 `\n` 拆行回调 `peer_uart_line_cb_t`。发送用 `peer_uart_link_send_line()`，带 TX 互斥锁。

### 第 3 步：jdil 新建 peer_cnc_slave

实现 HOST 命令处理：

- `on_host_line()` 解析 `{"t":"gcode","line":"..."}`、`{"t":"jog",...}` 等，调用 `MotionController::Get().Execute()` 或 `KanjiVGController` 方法
- 在 `MotionController::Execute()` 执行过程中周期推送 `{"t":"status",...}` 和 `{"t":"pos",...}` 给交互 S3
- 定时 `ping` 任务周期性发 `{"t":"pong","ok":true}`

### 第 4 步：jdil 推送语音对话

在 `application.cc` 的 `protocol_->OnIncomingJson()` 回调中，收到 `tts`/`stt`/`llm` 时追加：

```cpp
// 示例：TTS 语句推送
if (strcmp(type->valuestring, "tts") == 0) {
    // ... 原有逻辑 ...
    if (strcmp(state->valuestring, "sentence_start") == 0) {
        auto text = cJSON_GetObjectItem(root, "text");
        if (cJSON_IsString(text)) {
            char buf[1024];
            snprintf(buf, sizeof(buf),
                "{\"t\":\"chat\",\"role\":\"assistant\",\"text\":\"%s\"}",
                text->valuestring);
            peer_uart_link_send_line(buf);
        }
    }
}
```

同理 STT 发 `{"t":"chat","role":"user","text":"..."}`，LLM 发 `{"t":"emotion","id":"..."}`。

在 `application.cc` 的 `HandleStateChanged()` 中，切换状态时追加：

```cpp
const char* voice_states[] = {"idle","connecting","listening","speaking"}; // 精简版
// 只推送 idle/connecting/listening/speaking
peer_uart_link_send_line("{\"t\":\"voice_state\",\"state\":\"" + ... + "\"}");
```

### 第 5 步：交互 S3 扩展 chat 解析

在 `peer_cnc_client.cc` 的 `on_peer_line()` 中追加：

```cpp
} else if (strcmp(t, "chat") == 0) {
    // 调用 display->SetChatMessage(role, text)
} else if (strcmp(t, "emotion") == 0) {
    // 调用 display->SetEmotion(emotion)
} else if (strcmp(t, "voice_state") == 0) {
    // 更新 UI 状态图标/文字
}
```

### 第 6 步：编译验证

两个工程分别编译：

```bash
# 交互 S3
cd xiaozhi-esp32-main
idf.py set-target esp32s3 && idf.py build

# 运动/语音 S3
cd xiaozhi-esp32-jdil/xiaozhi-esp32-main
idf.py set-target esp32s3 && idf.py build
```

## 完整数据流

```
触摸/WebUI → webui_cnc_bridge → peer_cnc_client.send_gcode()
    → UART {"t":"gcode","line":"G0 X10 Y10"}
    → peer_cnc_slave.on_host_line() → MotionController.Execute()
    → Stepper.PulseISR() → GPIO 步进脉冲
    → peer_cnc_slave 周期推送 {"t":"status","pct":45,"busy":true}
    ← UART
    ← peer_cnc_client.on_peer_line() → LVGL 进度条 + WebSocket 推送浏览器

语音对话:
  云端 LLM stream → jdil Application.OnIncomingJson("tts","sentence_start")
    → UART {"t":"chat","role":"assistant","text":"你好"}
    ← peer_cnc_client 解析 → display->SetChatMessage("assistant", "你好")
```

## 注意事项

- **UART 交叉连接**：交互 TX(17) → 运动 RX(9)，交互 RX(9) ← 运动 TX(17)，共 GND
- **引脚冲突**：jdil 的 STEP_ENABLE(原9) 和 LASER_PWM(原17) 必须改到其他空闲引脚
- **两个工程独立**：各有自己的 sdkconfig、分区表、固件，互不影响
- **交互 S3 WebUI 需要 Wi-Fi**：手机浏览器通过 Wi-Fi 访问 HTTP/WS；若不需要 WebUI 可去掉 Wi-Fi
- **运动 S3 必须联网**：小智云端 STT/LLM/TTS 依赖 MQTT 或 WebSocket
- **CNC 代码全部 header-only**：`stepper.h`、`motion_controller.h`、`stepping_engine.h` 等都在头文件内实现，需要 include 而非编译链接
