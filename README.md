# BlueHIDFlow Firmware

**BLE HID Mobile Automation Tool** — 开源移动端自动化测试工具，运行在 ESP32-S3 上，通过 BLE HID 控制移动设备。

## 项目简介

BlueHIDFlow 是一个基于 ESP32-S3 的嵌入式固件，通过 BLE-HID 技术模拟鼠标和键盘操作，实现对移动设备的物理级控制。

### 名称含义
- **Blue**: BLE 蓝牙技术
- **HID**: 人机交互设备（Human Interface Device）
- **Flow**: 测试流程自动化

## 方案特点

| 特点 | 说明 |
|------|------|
| **无侵入** | 通过 BLE-HID 模拟外设操作，对手机系统及软件零修改 |
| **跨平台** | Android、iOS、HarmonyOS 三大系统通用，无需适配 |
| **软硬件结合** | ESP32-S3 硬件 + 桌面端/后端软件，物理控制手机 |
| **简单高效** | 适合点击、滑动、输入等简单动作的自动化测试 |
| **可扩展** | 结合手机端软件可扩展更丰富的自动化玩法 |

## 核心功能

| 功能 | 说明 |
|------|------|
| **BLE HID 鼠标控制** | 模拟绝对坐标触摸，支持点击、滑动、校准 |
| **BLE HID 键盘控制** | 文字输入、按键、组合键、媒体键（音量、播放等） |
| **MQTT 远程控制** | 通过 MQTT 主题收发命令，支持云端控制 |
| **Web 配置** | AP 模式 + Captive Portal，无需串口即可配网 |

## 硬件准备

- ESP32-S3 开发板（推荐 ESP32-S3-N16R8）
- OV2640 摄像头模块（可选，当前固件中已禁用）

## 快速开始

```bash
# 1. 克隆仓库
git clone git@github.com:antgroup/BlueHIDFlow.git
cd BlueHIDFlow

# 2. 复制配置文件并修改
cp config.example.h include/config.h
# 编辑 include/config.h 填入你的 WiFi 和 MQTT 配置

# 3. 安装依赖
pio pkg install

# 4. 编译
pio run

# 5. 烧录
pio run --target upload

# 6. 串口监视
pio device monitor -b 115200
```

> 首次使用建议通过 Web 配置页面进行配网，无需修改 config.h。详见[设备配网](#设备配网)。

## 设备配网

设备首次上电会创建 AP 热点 `BlueHIDFlow-config`（密码：`bluehidflow`），连接后访问 `http://192.168.8.1` 进行配置：

| 配置项 | 说明 |
|--------|------|
| WiFi SSID | 手机热点或 WiFi 网络名称 |
| WiFi 密码 | WiFi 密码 |
| MQTT Broker | MQTT Broker 地址 |
| MQTT 端口 | MQTT 端口（默认 8883） |
| MQTT 用户名 | MQTT 认证用户名 |
| MQTT 密码 | MQTT 认证密码 |
| TLS CA 证书 | MQTT TLS 连接所需的 CA 证书（PEM 格式） |
| 主题前缀 | MQTT 主题前缀（默认 `bluehidflow/`） |
| 设备名称 | 自定义名称 |
| HID 灵敏度 | 像素/HID 单位 |
| HID 方向反转 | 适配不同手机 |

## MQTT 命令

设备通过 MQTT 主题通信，所有主题均以可配置前缀开头（默认 `bluehidflow/`）：

| 主题 | 方向 | 用途 |
|------|------|------|
| `{prefix}command/{device_id}` | 客户端 → 设备 | 发送控制命令 |
| `{prefix}response/{device_id}` | 设备 → 客户端 | 返回执行结果 |
| `{prefix}status/{device_id}` | 设备 → 客户端 | 设备状态上报 |
| `{prefix}log/{device_id}` | 设备 → 客户端 | 设备日志上报 |

### 常用命令

**鼠标模式：**
```json
{"action": "TAP", "params": {"x": 500, "y": 300}}
{"action": "SWIPE", "params": {"from_x": 500, "from_y": 1000, "to_x": 500, "to_y": 500}}
```

**键盘模式：**
```json
{"action": "INPUT", "params": {"text": "Hello"}}
{"action": "KEY_PRESS", "params": {"key": "ENTER"}}
{"action": "HOME", "params": {}}
```

> 完整 API 文档请参阅 [docs/API.md](docs/API.md)

## 项目结构

```
BlueHIDFlow/
├── src/                     # C++ 源代码
│   ├── main.cpp             # WiFi、MQTT 初始化，主循环
│   ├── ble_hid.cpp          # BLE HID 统一接口
│   ├── ble_hid_keyboard.cpp # BLE HID 键盘模式
│   ├── ble_hid_mouse.cpp    # BLE HID 鼠标实现
│   ├── command_handler.cpp  # 命令解析与执行
│   ├── command_registry.cpp # 命令注册表
│   ├── config_manager.cpp   # 配置管理（NVS 持久化）
│   ├── connection_manager.cpp # 连接管理（自动重连）
│   ├── mqtt_client.cpp      # MQTT 客户端
│   ├── web_server.cpp       # Web 配置服务
│   ├── led_control.cpp      # LED 状态指示
│   ├── camera_server.cpp    # 摄像头服务（当前已禁用）
│   ├── commands/            # 命令实现
│   └── ...
├── include/                 # 头文件
├── docs/                    # 技术文档
├── lib/                     # 外部库
├── platformio.ini           # PlatformIO 配置
├── config.example.h         # 配置文件模板
└── CLAUDE.md                # AI 开发助手指南
```

## 文档

- [API 文档](docs/API.md) - 完整的 MQTT 消息格式和命令说明
- [硬件接线指南](HARDWARE_SETUP_GUIDE.zh-CN.md) - ESP32-S3 接线与调试
- [贡献指南](CONTRIBUTING.md) - 如何参与项目开发

## 贡献

欢迎贡献！请阅读 [CONTRIBUTING.md](CONTRIBUTING.md) 了解贡献指南。

## 许可证

本项目基于 [Apache License 2.0](LICENSE) 开源。

## 已知限制

- 鼠标模式在部分手机上受鼠标加速影响，点击不准确
- HID 模式切换需要重启设备（约 5-8 秒）
- 摄像头功能当前已禁用，待后续版本完善
