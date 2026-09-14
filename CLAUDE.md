# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 语言偏好

**使用中文进行对话和注释**。所有交流、代码注释、文档说明均使用中文。

## Project Overview

BlueHIDFlow 是一个开源嵌入式固件项目，运行在 ESP32-S3 上，通过 BLE HID 控制移动设备（手机/平板），通过 MQTT 与上位机通信。用户可自行搭配任意 MQTT broker 和前端控制程序使用。

## Project Structure

```
BlueHIDFlow/
├── src/                     # C++ 源代码
│   ├── main.cpp             # WiFi、MQTT 初始化，主循环
│   ├── ble_hid.cpp          # BLE HID 统一接口（键盘/鼠标）
│   ├── ble_hid_keyboard.cpp # BLE HID 键盘模式
│   ├── ble_hid_mouse.cpp    # BLE HID 鼠标实现
│   ├── camera_server.cpp    # 摄像头服务（当前已禁用）
│   ├── command_handler.cpp  # 命令解析与执行
│   ├── command_registry.cpp # 命令注册表
│   ├── config_manager.cpp   # 配置管理（NVS 持久化）
│   ├── connection_manager.cpp # 连接管理（自动重连）
│   ├── mqtt_client.cpp      # MQTT 客户端（TLS）
│   ├── web_server.cpp       # AP 模式 Web 配置界面
│   ├── led_control.cpp      # LED 状态指示
│   ├── commands/            # 命令实现（鼠标/键盘/系统/配置）
│   ├── config_providers/    # 配置序列化/反序列化
│   └── ...
├── include/                 # 头文件
│   ├── config.h             # 设备配置（WiFi/MQTT 凭据，由开发者自行填写）
│   ├── config.example.h     # 配置文件模板
│   ├── gpio_config.h        # GPIO 引脚分配
│   ├── icamera_uploader.h   # 摄像头图片上传接口（开发者自行实现）
│   ├── imqtt_publisher.h    # MQTT 发布接口
│   └── ...
├── docs/                    # 技术文档
├── lib/                     # 外部库
├── platformio.ini           # PlatformIO 配置
└── config.example.h         # 配置文件模板
```

## Common Development Commands

```bash
# 安装依赖
pio pkg install

# 编译
pio run

# 烧录
pio run --target upload

# 清理构建
pio run --target clean

# 串口监视器
pio device monitor -b 115200
```

## Architecture

### Communication Flow

```
上位机/前端 ──── MQTT ─────> MQTT Broker (用户自选)
                                │
                                v
                Embedded (ESP32-S3) <─ MQTT
                                     │
                                     v (BLE HID)
                               手机/平板设备
```

### MQTT Topic Structure

| Topic | Direction | QoS | Purpose |
|-------|-----------|------|---------|
| `bluehidflow/command/{device_id}` | 上位机 → Embedded | 2 | Send tap/input/swipe/wait commands |
| `bluehidflow/response/{device_id}` | Embedded → 上位机 | 1 | Command execution results |
| `bluehidflow/status/{device_id}` | Embedded → 上位机 | 1 | Device state (online/offline/busy, battery, WiFi) |
| `bluehidflow/log/{device_id}` | Embedded → 上位机 | 0 | Device log reporting |

**Command format** (上位机 → Embedded):
```json
{
  "message_id": "uuid",
  "timestamp": "2026-03-17T10:00:00Z",
  "action": "TAP|INPUT|SWIPE|WAIT|REBOOT",
  "params": { "x": 100, "y": 200 }
}
```

### Implemented Commands

| Command | Mode | Description |
|---------|------|-------------|
| `TAP` | Mouse | Absolute coordinate tap |
| `SWIPE` | Mouse | Absolute coordinate swipe |
| `SWIPE_UP/DOWN/LEFT/RIGHT` | Mouse | Directional swipe |
| `CALIBRATE` | Mouse | Recalibrate HID coordinate system |
| `INPUT` | Keyboard | Text input |
| `KEY_PRESS` | Keyboard | Single key press |
| `KEY_COMBO` | Keyboard | Key combination |
| `HOME` | Keyboard | Home key |
| `BACK` | Keyboard | Back key |
| `VOLUME_UP` | Keyboard | Volume up media key |
| `VOLUME_DOWN` | Keyboard | Volume down media key |
| `WAIT` | Any | Wait specified duration |
| `GET_STATUS` | Any | Get device status |
| `STATUS` | Any | Print status to serial |
| `RECONNECT` | Any | Force WiFi/MQTT reconnect |
| `SWITCH_MODE` | Any | Switch HID mode (keyboard/mouse) |
| `GET_CONFIG` | Any | Get configuration |
| `SET_CONFIG` | Any | Set configuration |
| `CONFIG` | Any | Get/Set configuration (combined) |

### Currently Implemented (v0.9.0)

- WiFi connection management with auto-reconnect and AP fallback
- MQTT client with TLS (port 8883), command/response/status/log topics
- **BLE HID Mouse mode** (Android/HarmonyOS compatible)
- **BLE HID Keyboard mode**: text input, key press, media keys, key combos
- **Command parsing and execution**: full command set (see table above)
- **Web configuration (AP mode)**: Captive Portal at 192.168.8.1
- Camera HTTP server skeleton (OV2640, currently disabled in firmware)
- LED status indicator (RGB LED)
- Periodic status and log publishing via MQTT
- Configurable MQTT topic prefix

## Configuration

### Web Configuration (AP Mode)

设备首次上电会创建 AP 热点 `BlueHIDFlow-config`（密码：`bluehidflow`），连接后访问 `http://192.168.8.1` 进行配置：

| 配置项 | 说明 |
|--------|------|
| WiFi SSID | 手机热点或 WiFi 网络名称 |
| MQTT Broker | MQTT broker 地址 |
| MQTT TLS CA 证书 | TLS 连接所需的 CA 证书（PEM 格式） |
| HID 灵敏度 | 像素/HID 单位 |
| HID 方向反转 | 适配不同手机 |

### Required Libraries

NimBLE-Arduino, PubSubClient, ArduinoJson, esp32-camera (PlatformIO manages dependencies)

## Important Notes

- MQTT broker 使用 TLS (port 8883)，CA 证书由开发者通过 config.h 或 Web 配置页面提供
- Device ID is stored in ESP32 Preferences/NVS (auto-generated from MAC address)
- HID sensitivity and direction are configurable via Web portal and persisted in Preferences
- Camera upload logic is abstracted via `ICameraUploader` interface — developers inject their own implementation
