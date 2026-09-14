# BlueHIDFlow API 文档

本文档描述 BlueHIDFlow 嵌入式设备与客户端之间通过 MQTT 进行通信的消息格式和接口规范。

---

## 目录

1. [协议概述](#1-协议概述)
2. [MQTT 配置信息](#2-mqtt-配置信息)
3. [MQTT 主题结构](#3-mqtt-主题结构)
4. [命令消息（Command）](#4-命令消息 command)
5. [摄像头消息（Camera）](#5-摄像头消息 camera)
6. [响应消息（Response）](#6-响应消息 response)
7. [状态消息（Status）](#7-状态消息 status)
8. [日志消息（Log）](#8-日志消息 log)
9. [命令执行结果码](#9-命令执行结果码)
10. [使用示例](#10-使用示例)

---

## 1. 协议概述

### 1.1 协议版本

当前协议版本：**1.0**

BlueHIDFlow 支持两种命令格式：

**新格式（推荐）**：
```json
{
  "version": "1.0",
  "id": "uuid-v4",
  "timestamp": 1716123456789,
  "action": "tap",
  "params": { "x": 500, "y": 300 },
  "metadata": { "sessionId": "session-123" }
}
```

**旧格式（向后兼容）**：
```json
{
  "action": "TAP",
  "message_id": "12345678",
  "params": { "x": 500, "y": 300 }
}
```

> 设备同时支持两种格式，旧格式默认使用协议版本 1.0。

### 1.2 字段映射

| 新格式字段 | 旧格式字段 | 说明 |
|-----------|-----------|------|
| `id` | `message_id` | 消息唯一标识 |
| `version` | - | 协议版本（可选，默认 "1.0"） |
| `timestamp` | - | 消息时间戳（可选） |
| `metadata.sessionId` | - | 会话标识（可选） |

---

## 2. MQTT 配置信息

### 2.1 Broker 连接参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| **Broker 地址** | 可配置 | MQTT Broker 地址 |
| **端口** | `8883` | TLS 加密端口 |
| **协议** | `mqtts://` | 加密协议 |
| **用户名** | 可配置 | MQTT 认证用户名 |
| **密码** | 可配置 | MQTT 认证密码 |
| **KeepAlive** | `60 秒` | 连接保活间隔 |
| **QoS** | `1` | 至少一次投递 |

### 2.2 TLS/SSL 证书

使用 **DigiCert Global Root G2** CA 证书验证服务器身份。

### 2.3 设备 ID 生成规则

设备 ID 始终使用 WiFi MAC 地址自动生成（如 `1CDBD44B0308`）。MAC 地址获取失败时使用 `udid-XXXXXX`（时间戳后 6 位）。

---

## 3. MQTT 主题结构

所有主题均以可配置的主题前缀开头（默认 `bluehidflow/`），以设备 ID 为后缀，实现设备隔离。

| 主题 | 方向 | QoS | Retain | 用途 |
|------|------|-----|--------|------|
| `{prefix}command/{device_id}` | 客户端 → 设备 | 2 | 否 | 发送控制命令 |
| `{prefix}response/{device_id}` | 设备 → 客户端 | 1 | 否 | 返回命令执行结果 |
| `{prefix}status/{device_id}` | 设备 → 客户端 | 1 | 是 | 设备状态上报 |
| `{prefix}log/{device_id}` | 设备 → 客户端 | 1 | 否 | 设备日志上报 |
| `{prefix}camera/{device_id}` | 设备 → 客户端 | 1 | 否 | 摄像头数据上报 |

**主题说明：**
- `{prefix}`：主题前缀，默认 `bluehidflow/`，可通过 Web 配置修改
- `{device_id}`：设备唯一标识，例如 `1CDBD44B0308`
- 方向：描述消息的发送方向
- QoS：消息服务质量等级
- Retain：是否保留最新消息（新订阅者立即获取）

---

## 4. 命令消息（Command）

### 4.1 通用消息格式

上位机发送命令到 `{prefix}command/{device_id}` 主题。

**新格式（推荐）**：
```json
{
  "version": "1.0",
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "timestamp": 1716123456789,
  "action": "tap",
  "params": {
    "x": 500,
    "y": 300
  },
  "metadata": {
    "sessionId": "session-123"
  }
}
```

**旧格式（兼容）**：
```json
{
  "action": "TAP",
  "message_id": "12345678",
  "params": {
    "x": 500,
    "y": 300
  }
}
```

**字段说明：**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `version` | String | 否 | 协议版本，默认 "1.0" |
| `id` | String | 是* | 消息唯一 ID（新格式） |
| `message_id` | String | 是* | 消息唯一 ID（旧格式，建议使用 `id`） |
| `timestamp` | Long | 否 | 消息时间戳（毫秒） |
| `action` | String | 是 | 命令类型（大小写不敏感） |
| `params` | Object | 否 | 命令参数，结构因 action 而异 |
| `metadata` | Object | 否 | 元数据，包含 sessionId 等 |

*注：`id` 和 `message_id` 二选一，推荐使用 `id`

### 4.2 支持的命令类型

#### 鼠标模式命令（需切换到鼠标模式）

| 命令 | 说明 | params 参数 |
|------|------|-------------|
| `TAP` | 点击屏幕 | `x`, `y`, `duration`（可选） |
| `SWIPE` | 滑动屏幕 | `from_x`, `from_y`, `to_x`, `to_y`, `duration`（可选） |
| `SWIPE_UP` | 向上滑动 | `duration`（可选） |
| `SWIPE_DOWN` | 向下滑动 | `duration`（可选） |
| `SWIPE_LEFT` | 向左滑动 | `duration`（可选） |
| `SWIPE_RIGHT` | 向右滑动 | `duration`（可选） |
| `CALIBRATE` | 校准/配置 | `sensitivity`, `move_delay`, `hid_x`, `hid_y` 等 |

#### 键盘模式命令（需切换到键盘模式）

| 命令 | 说明 | params 参数 |
|------|------|-------------|
| `INPUT` | 输入文本 | `text`, `confirm`（可选） |
| `KEY_PRESS` | 按键 | `key` |
| `KEY_COMBO` | 组合键 | `key`, `modifiers` |
| `HOME` | 回到桌面 | 无 |
| `BACK` | 返回 | 无 |
| `VOLUME_UP` | 音量增大 | 无 |
| `VOLUME_DOWN` | 音量减小 | 无 |

#### 通用命令（任意模式可用）

| 命令 | 说明 | params 参数 |
|------|------|-------------|
| `SWITCH_MODE` | 切换 HID 模式 | `mode` |
| `WAIT` | 等待 | `duration` |
| `STATUS` | 查询状态（仅串口输出，不发 MQTT 响应） | 无 |
| `GET_CONFIG` | 获取配置 | `section`（可选） |
| `SET_CONFIG` | 设置配置 | 顶层配置节对象 |
| `RECONNECT` | 重新连接 MQTT（未实现） | 无 |
| `CONFIG` | 配置页面请求（未实现） | 无 |
| `GET_STATUS` | 获取详细状态（未实现） | 无 |

#### 摄像头命令（当前已禁用）

| 命令 | 说明 | params 参数 |
|------|------|-------------|
| ~~`CAPTURE`~~ | ~~拍照并上传~~ | 无 |
| ~~`CAMERA_START`~~ | ~~开始定时拍照~~ | `interval`（可选） |
| ~~`CAMERA_STOP`~~ | ~~停止定时拍照~~ | 无 |
| ~~`CAMERA_STATUS`~~ | ~~获取摄像头状态~~ | 无 |

> **注意**：摄像头命令在当前固件版本中已注释禁用，发送后将返回 `INVALID_PARAMS`。

---

### 4.3 命令详细说明

#### 4.3.1 TAP - 点击屏幕（鼠标模式）

在指定坐标执行点击操作。

**params 参数：**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `x` | Integer | 是 | X 坐标（像素） |
| `y` | Integer | 是 | Y 坐标（像素） |
| `duration` | Integer | 否 | 点击持续时间（毫秒），默认 50ms |

**示例：**
```json
{
  "action": "TAP",
  "message_id": "12345678",
  "params": {
    "x": 500,
    "y": 300,
    "duration": 100
  }
}
```

---

#### 4.3.2 INPUT - 输入文本（键盘模式）

通过 BLE 键盘输入文本。

**params 参数：**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `text` | String | 是 | 要输入的文本内容 |
| `confirm` | String | 否 | 输入后按确认键：`"space"` 按空格，`"enter"` 按回车 |

**示例（纯文本输入）：**
```json
{
  "action": "INPUT",
  "message_id": "12345679",
  "params": {
    "text": "Hello World"
  }
}
```

**示例（输入后按回车确认）：**
```json
{
  "action": "INPUT",
  "message_id": "12345679",
  "params": {
    "text": "hello",
    "confirm": "enter"
  }
}
```

**已知限制：** 中文输入法下数字键可能被候选词选择器拦截，建议切换英文输入法。

---

#### 4.3.3 KEY_PRESS - 按键（键盘模式）

发送单个按键事件。

**params 参数：**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `key` | String | 是 | 按键名称（见下表） |

**支持的按键名称：**

| 分类 | 按键名 | 说明 |
|------|--------|------|
| **导航** | `ENTER` | 回车 |
| | `ESCAPE` | ESC 键 |
| | `BACKSPACE` | 退格键 |
| | `DELETE` | 删除键 |
| | `TAB` | Tab 键 |
| | `SPACE` | 空格键 |
| **方向** | `UP` | 方向键上 |
| | `DOWN` | 方向键下 |
| | `LEFT` | 方向键左 |
| | `RIGHT` | 方向键右 |
| **翻页** | `PAGE_UP` | 向上翻页（Android 部分场景无效） |
| | `PAGE_DOWN` | 向下翻页（Android 部分场景无效） |
| **功能键** | `F1` - `F12` | 功能键（Android 上用途有限） |
| **媒体键** | `MUTE` | 静音切换 |
| | `PLAY_PAUSE` | 播放/暂停 |
| | `NEXT_TRACK` | 下一曲 |
| | `PREV_TRACK` | 上一曲 |
| **浏览器键** | `BROWSER_BACK` | 浏览器后退（等同于 BACK） |
| | `BROWSER_HOME` | 浏览器主页（等同于 HOME） |
| **焦点激活** | `DPAD_CENTER` | 焦点确认（Consumer AC Select，尝试替代 ENTER 激活焦点元素） |
| **字母/数字** | `A`-`Z`, `0`-`9` | 单字符按键（直接发送 HID 扫描码） |

**示例：**
```json
{
  "action": "KEY_PRESS",
  "message_id": "12345680",
  "params": {
    "key": "TAB"
  }
}
```

---

#### 4.3.4 KEY_COMBO - 组合键（键盘模式）

发送修饰键 + 按键的组合。

**params 参数：**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `key` | String | 是 | 按键名称（同 KEY_PRESS） |
| `modifiers` | Array | 是 | 修饰键列表，可选值：`"ctrl"`, `"shift"`, `"alt"`, `"gui"` |

**示例（Ctrl+C 复制）：**
```json
{
  "action": "KEY_COMBO",
  "message_id": "12345681",
  "params": {
    "key": "C",
    "modifiers": ["ctrl"]
  }
}
```

**示例（Ctrl+Shift+A 多修饰键）：**
```json
{
  "action": "KEY_COMBO",
  "message_id": "12345682",
  "params": {
    "key": "A",
    "modifiers": ["ctrl", "shift"]
  }
}
```

**注意：** 媒体键不支持组合键修饰，发送媒体键的 KEY_COMBO 会忽略修饰键直接发送媒体键。

---

#### 4.3.5 HOME - 回到桌面（键盘模式）

发送 AC Home 媒体键，回到手机桌面。

**params 参数：** 无

**示例：**
```json
{
  "action": "HOME",
  "message_id": "12345683"
}
```

---

#### 4.3.6 BACK - 返回（键盘模式）

发送 AC Back 媒体键，返回上一页。

**params 参数：** 无

**示例：**
```json
{
  "action": "BACK",
  "message_id": "12345684"
}
```

---

#### 4.3.7 VOLUME_UP / VOLUME_DOWN - 音量控制（键盘模式）

发送音量增减媒体键。

**params 参数：** 无

**示例：**
```json
{
  "action": "VOLUME_UP",
  "message_id": "12345685"
}
```

---

#### 4.3.8 SWITCH_MODE - 切换 HID 模式

切换键盘/鼠标模式，切换后设备自动重启（约 5-8 秒）。

**params 参数：**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `mode` | String | 是 | 模式名称：`"keyboard"` 或 `"mouse"` |

**示例：**
```json
{
  "action": "SWITCH_MODE",
  "message_id": "12345686",
  "params": {
    "mode": "mouse"
  }
}
```

**注意：** 模式切换通过 NVS 保存 + `ESP.restart()` 实现，设备会断开连接约 5-8 秒后重启。

---

#### 4.3.9 SWIPE - 滑动屏幕（鼠标模式）

从起点滑动到终点。

**params 参数：**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `from_x` | Integer | 是 | 起始 X 坐标 |
| `from_y` | Integer | 是 | 起始 Y 坐标 |
| `to_x` | Integer | 是 | 结束 X 坐标 |
| `to_y` | Integer | 是 | 结束 Y 坐标 |
| `duration` | Integer | 否 | 滑动持续时间（毫秒），默认 300ms |

**示例：**
```json
{
  "action": "SWIPE",
  "message_id": "12345687",
  "params": {
    "from_x": 500,
    "from_y": 500,
    "to_x": 200,
    "to_y": 500,
    "duration": 500
  }
}
```

---

#### 4.3.10 SWIPE_* - 方向滑动（鼠标模式）

向指定方向滑动（从屏幕中心开始）。

**params 参数：**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `duration` | Integer | 否 | 滑动持续时间（毫秒），默认 300ms |

**示例（向上滑动）：**
```json
{
  "action": "SWIPE_UP",
  "message_id": "12345688",
  "params": {
    "duration": 400
  }
}
```

---

#### 4.3.11 CALIBRATE - 校准/配置（鼠标模式）

配置 HID 参数或执行校准操作。

**params 参数（可选其一或多个）：**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `sensitivity` | Float | 否 | HID 灵敏度（像素/HID 单位） |
| `move_delay` | Integer | 否 | HID 移动延迟（毫秒） |
| `hid_x` | Integer | 否 | HID X 坐标（用于测试） |
| `hid_y` | Integer | 否 | HID Y 坐标（用于测试） |
| `delay_ms` | Integer | 否 | 测试延迟（毫秒） |
| `step_by_step` | Boolean | 否 | 是否分步执行测试 |

**示例（设置灵敏度）：**
```json
{
  "action": "CALIBRATE",
  "message_id": "12345689",
  "params": {
    "sensitivity": 1.5
  }
}
```

---

#### 4.3.12 WAIT - 等待

等待指定时间。

**params 参数：**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `duration` | Integer | 是 | 等待时间（毫秒） |

**示例：**
```json
{
  "action": "WAIT",
  "message_id": "12345690",
  "params": {
    "duration": 5000
  }
}
```

---

#### 4.3.13 STATUS - 查询状态

查询设备当前状态。**注意：此命令仅通过串口输出状态信息，不发送 MQTT 响应。**

**params 参数：** 无

**响应方式：** 串口打印（Serial），不通过 MQTT 返回

**串口输出示例：**
```
CMD: ===== 设备状态 =====
CMD: HID 模式: KEYBOARD
CMD: BLE 连接: 已连接
CMD: BLE 配对: 已配对
CMD: 屏幕尺寸: 1220 x 2700
CMD: HID 灵敏度: 1.00 像素/HID单位
CMD: HID 移动延迟: 50ms
CMD: ====================
```

**示例：**
```json
{
  "action": "STATUS",
  "message_id": "12345691"
}
```

---

#### 4.3.14 GET_CONFIG - 获取配置

获取设备配置信息。

**params 参数：**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `section` | String | 否 | 配置节名称（`wifi`, `camera`, `mqtt`, `ai`, `bluetooth`, `phone`, `system`），不传则返回全部 |

**示例（获取手机配置）：**
```json
{
  "action": "GET_CONFIG",
  "message_id": "12345692",
  "params": {
    "section": "phone"
  }
}
```

---

#### 4.3.15 SET_CONFIG - 设置配置

设置设备配置。

**params 参数：**

直接将配置节作为顶层键传入，支持同时设置多个配置节。

| 配置节 | 说明 | 主要字段 |
|--------|------|----------|
| `wifi` | WiFi 配置 | `ssid`, `password` |
| `camera` | 摄像头配置 | `resolution`, `jpegQuality`, `distanceCm` |
| `mqtt` | MQTT 配置 | `broker`, `port`, `username`, `password`, `caCert`, `deviceId` |
| `ai` | 大模型配置 | `apiKey`, `apiUrl`, `model`, `timeoutMs` |
| `bluetooth` | 蓝牙配置 | `deviceName`, `hidMode` |
| `phone` | 手机配置 | `model`, `screenWidth`, `screenHeight`, `hidSensitivity`, `moveDelayMs` |
| `system` | 系统配置 | `logLevel`, `debugEnabled` |

**示例（设置手机配置）：**
```json
{
  "action": "SET_CONFIG",
  "message_id": "12345693",
  "params": {
    "phone": {
      "model": "HUAWEI P60",
      "screenWidth": 1220,
      "screenHeight": 2700,
      "hidSensitivity": 1.0
    }
  }
}
```

---

#### 4.3.16 RECONNECT - 重新连接

重新连接 MQTT Broker。

**params 参数：** 无

**示例：**
```json
{
  "action": "RECONNECT",
  "message_id": "12345694"
}
```

---

## 5. 摄像头消息（Camera）

> **当前状态**：摄像头命令已在固件中禁用。以下文档保留供参考。

设备拍照后，通过**可注入的图片上传接口**（`ICameraUploader`）将图片上传到开发者自有的后端/对象存储，然后通过 `bluehidflow/camera/{device_id}` 主题发布图片 URL。

> **安全说明**：上传逻辑已抽象为 `ICameraUploader` 接口，开发者通过 `cameraServer.setUploader(...)` 注入自定义实现，不再内置任何第三方上传端点或 Token，避免泄露私有实现与凭据。

### 4.1 摄像头消息格式

```json
{
  "url": "https://your-storage.example.com/path/to/frame.jpg",
  "timestamp": 12345678
}
```

**字段说明：**

| 字段 | 类型 | 说明 |
|------|------|------|
| `url` | String | 图片访问 URL（由上传器返回，开发者自行管理鉴权） |
| `timestamp` | Integer | 拍照时间戳（毫秒） |

---

## 6. 响应消息（Response）

设备执行命令后，发布响应到 `bluehidflow/response/{device_id}` 主题。

### 5.1 通用响应格式

```json
{
  "message_id": "时间戳（与命令对应）",
  "status": "执行状态",
  "timestamp": 12345678,
  "error": "错误信息（可选）",
  "data": { ... }
}
```

**字段说明：**

| 字段 | 类型 | 说明 |
|------|------|------|
| `message_id` | String | 对应命令的 message_id（时间戳） |
| `status` | String | 执行状态：`PROCESSING`, `SUCCESS`, `FAILED` |
| `timestamp` | Integer | 响应生成的设备毫秒时间戳 |
| `error` | String | 错误信息（失败时） |
| `data` | Object | 附加数据（如 GET_CONFIG 返回的配置） |

### 5.2 响应示例

**命令执行成功：**
```json
{
  "message_id": "12345678",
  "status": "SUCCESS",
  "timestamp": 12345780
}
```

**命令执行失败：**
```json
{
  "message_id": "12345678",
  "status": "FAILED",
  "timestamp": 12345780,
  "error": "坐标超出屏幕范围"
}
```

**GET_CONFIG 响应（带数据）：**
```json
{
  "message_id": "12345686",
  "status": "SUCCESS",
  "timestamp": 12345800,
  "data": {
    "wifi": {
      "ssid": "MyWiFi",
      "password": "******"
    },
    "phone": {
      "model": "HUAWEI P60",
      "screenWidth": 1220,
      "screenHeight": 2700,
      "hidSensitivity": 1.0
    }
  }
}
```

---

## 7. 状态消息（Status）

设备定期发布状态到 `bluehidflow/status/{device_id}` 主题（默认 30 秒间隔），新订阅者会立即收到最新状态（retain=true）。

**注意**：状态上报的前提是 MQTT 已连接成功。如果网络断开或 MQTT 未连接，则不会发布状态消息。

### 6.1 状态消息格式

```json
{
  "device_id": "1CDBD44B0308",
  "status": "connected",
  "battery": 85,
  "wifi_rssi": -45,
  "uptime": 12345,
  "ip": "192.168.1.100",
  "mac": "1C:DB:D4:4B:03:08"
}
```

**字段说明：**

| 字段 | 类型 | 说明 |
|------|------|------|
| `device_id` | String | 设备唯一标识（WiFi MAC 地址） |
| `status` | String | 设备状态：`offline`, `connecting`, `connected`, `busy` |
| `battery` | Integer | 电池电量百分比（当前固定值 85） |
| `wifi_rssi` | Integer | WiFi 信号强度（dBm），负数，越接近 0 信号越强 |
| `uptime` | Integer | 设备运行时间（秒） |
| `ip` | String | 设备 IP 地址 |
| `mac` | String | 设备 MAC 地址 |

---

## 8. 日志消息（Log）

设备定期上报日志到 `bluehidflow/log/{device_id}` 主题（默认 30 秒间隔，或错误时立即上报）。

### 7.1 日志消息格式

```json
[
  {
    "ts": 12345678,
    "level": "日志级别",
    "event": "事件类型",
    "msg": "日志内容"
  }
]
```

**字段说明：**

| 字段 | 类型 | 说明 |
|------|------|------|
| `ts` | Integer | 设备毫秒时间戳 |
| `level` | String | 日志级别：`INFO`, `WARN`, `ERROR` |
| `event` | String | 事件类型（见下表） |
| `msg` | String | 日志内容 |

### 7.2 事件类型

| 事件 | 说明 |
|------|------|
| `BOOT` | 设备启动 |
| `WIFI_CONNECT` | WiFi 连接 |
| `WIFI_DISCONNECT` | WiFi 断开 |
| `MQTT_CONNECT` | MQTT 连接 |
| `MQTT_DISCONNECT` | MQTT 断开 |
| `BLE_CONNECT` | BLE 连接 |
| `BLE_DISCONNECT` | BLE 断开 |
| `ERROR` | 异常错误 |
| `RESTART` | 设备重启 |
| `CONFIG_SAVE` | 配置保存 |

---

## 9. 命令执行结果码

设备内部使用的命令执行结果码：

| 结果码 | 值 | 说明 |
|--------|-----|------|
| `SUCCESS` | 0 | 命令执行成功 |
| `FAILED` | 1 | 命令执行失败 |
| `INVALID_PARAMS` | 2 | 参数无效 |
| `NOT_IMPLEMENTED` | 3 | 功能未实现 |
| `TIMEOUT` | 4 | 命令超时 |
| `RESPONSE_SENT` | 5 | 命令已自行发送 MQTT 响应，execute() 末尾无需再发 |

> `RESPONSE_SENT` 用于 STATUS、GET_CONFIG、SET_CONFIG、SWITCH_MODE 等需要返回自定义数据的命令，它们在函数内部已直接调用 `mqtt.publishResponse()`。

---

## 10. 使用示例

### 9.1 使用 MQTTX 测试

1. 连接 MQTT Broker：
   - Host: `your-mqtt-broker.example.com`
   - Port: `8883`
   - SSL/TLS: 开启
   - Username: `your_username`
   - Password: `your_password`

2. 订阅设备响应主题：
   ```
   bluehidflow/response/1CDBD44B0308
   ```

3. 发布键盘按键命令：
   ```json
   {
     "action": "KEY_PRESS",
     "message_id": "12345678",
     "params": {
       "key": "ENTER"
     }
   }
   ```

4. 查看响应：
   ```json
   {
     "message_id": "12345678",
     "status": "SUCCESS",
     "timestamp": 12345780
   }
   ```

### 9.2 使用 Python 脚本

```python
import paho.mqtt.client as mqtt
import json
import ssl

# 配置
BROKER = "your-mqtt-broker.example.com"
PORT = 8883
USERNAME = "your_username"
PASSWORD = "your_password"
DEVICE_ID = "1CDBD44B0308"

# 回调函数
def on_connect(client, userdata, flags, rc):
    print(f"已连接，结果码：{rc}")
    client.subscribe(f"bluehidflow/response/{DEVICE_ID}")

def on_message(client, userdata, msg):
    print(f"收到响应：{msg.payload.decode()}")

# 创建客户端
client = mqtt.Client()
client.username_pw_set(USERNAME, PASSWORD)
client.tls_set(ca_certs=None, certfile=None, keyfile=None, cert_reqs=ssl.CERT_REQUIRED)

# 设置回调
client.on_connect = on_connect
client.on_message = on_message

# 连接
client.connect(BROKER, PORT, 60)

# 发布按键命令
command = {
    "action": "KEY_PRESS",
    "message_id": str(int(__import__('time').time() * 1000)),
    "params": {
        "key": "HOME"
    }
}
client.publish(f"bluehidflow/command/{DEVICE_ID}", json.dumps(command))
print(f"已发布命令：{command}")

# 循环接收
client.loop_forever()
```

### 9.3 完整工作流程

```
上位机                          设备                          Broker
  |                              |                              |
  |--- 订阅 response 主题 ------->|                              |
  |                              |--- 订阅 command 主题 -------->|
  |                              |                              |
  |--- 发布 KEY_PRESS 命令 ------------------------------------->|
  |                              |<-----------------------------|
  |                              | 处理命令 (发送按键)           |
  |                              |                              |
  |<-- 返回 SUCCESS 响应 --------------------------------------|
  |                              |                              |
  |                              | 定期发布状态 (每 30 秒) ------->|
  |<-- 收到 status 消息 ---------------------------------------|
```

---

## 附录

### A. 配置节说明

| 配置节 | 说明 | 主要字段 |
|--------|------|----------|
| `wifi` | WiFi 配置 | `ssid`, `password` |
| `camera` | 摄像头配置 | `resolution`, `jpegQuality`, `distanceCm` |
| `mqtt` | MQTT 配置 | `broker`, `port`, `username`, `password`, `caCert`, `deviceId` |
| `ai` | 大模型配置 | `apiKey`, `apiUrl`, `model`, `timeoutMs` |
| `bluetooth` | 蓝牙配置 | `deviceName`, `hidMode` |
| `phone` | 手机配置 | `model`, `screenWidth`, `screenHeight`, `hidSensitivity`, `moveDelayMs` |
| `system` | 系统配置 | `logLevel`, `debugEnabled` |

### B. HID 模式说明

| 模式值 | 名称 | 说明 |
|--------|------|------|
| 0 | `KEYBOARD` | 键盘模式（默认）。支持文字输入、按键、组合键、导航键、媒体键 |
| 1 | `MOUSE` | 鼠标模式。通过归零 + 相对移动模拟点击和滑动 |

> **注意**：两种模式不能同时运行，切换需要重启设备（约 5-8 秒）。使用 `SWITCH_MODE` 命令切换。

### C. 按键映射参考

> **注意**：代码内部使用 HijelHID 库的常量映射，实际 HID 报告使用库内部定义的值。以下表格为标准 HID 规范参考。

#### 普通键（标准 HID Keyboard Page 0x07）

| KEY_PRESS 参数 | 标准 HID 扫描码 | Android 行为 |
|----------------|-----------|-------------|
| `ENTER` | 0x28 | 回车/确认 |
| `ESCAPE` | 0x29 | ESC（非返回） |
| `BACKSPACE` | 0x2A | 退格删除 |
| `DELETE` | 0x4C | 向后删除 |
| `TAB` | 0x2B | 焦点跳转 |
| `SPACE` | 0x2C | 空格 |
| `UP`/`DOWN`/`LEFT`/`RIGHT` | 0x52/0x51/0x50/0x4F | 方向键/焦点移动 |
| `PAGE_UP`/`PAGE_DOWN` | 0x4B/0x4E | 翻页（部分场景无效） |
| `F1`-`F12` | 0x3A-0x45 | 功能键（Android 用途有限） |
| `A`-`Z` | 0x04-0x1D | 字母键 |
| `0`-`9` | 0x27,0x1E-0x26 | 数字键 |

#### 媒体键（HID Consumer Page 0x0C）

| KEY_PRESS 参数 | Consumer Usage | Android 行为 |
|----------------|---------------|-------------|
| `HOME` | AC Home (0x0223) | 回桌面 |
| `BACK` | AC Back (0x0224) | 返回上一页 |
| `VOLUME_UP` | Volume Up (0x00E9) | 音量增大 |
| `VOLUME_DOWN` | Volume Down (0x00EA) | 音量减小 |
| `MUTE` | Mute (0x00E2) | 静音切换 |
| `PLAY_PAUSE` | Play/Pause (0x00CD) | 播放/暂停 |
| `NEXT_TRACK` | Next Track (0x00B5) | 下一曲 |
| `PREV_TRACK` | Prev Track (0x00B6) | 上一曲 |
| `DPAD_CENTER` | AC Select (0x0214) | 焦点确认（实验性） |

#### 修饰键（KEY_COMBO 使用）

| 修饰键名称 | HID 修饰码 | 说明 |
|-----------|-----------|------|
| `ctrl` | 0x01 | Ctrl / Control |
| `shift` | 0x02 | Shift |
| `alt` | 0x04 | Alt |
| `gui` | 0x08 | Win / Cmd / Meta |

### D. 常见问题

**Q: 消息发送后没有收到响应？**
- 检查设备是否在线（查看 status 主题）
- 确认 `message_id` 是否正确
- 检查 QoS 设置（command 主题建议使用 QoS 2）

**Q: KEY_PRESS 的 ENTER 在 app 内无法点击按钮？**
- Android 上 ENTER 键（HID 0x28）映射为 KEYCODE_ENTER，不等同于 DPAD_CENTER
- 系统标准控件（如桌面图标）通常响应 ENTER，但自定义 View 可能不响应
- 可尝试 `DPAD_CENTER` 键（实验性），或切换到鼠标模式使用 TAP

**Q: 如何重置设备配置？**
- 通过 Web 配置页面的"重置所有配置"按钮
- 或发送 `SET_CONFIG` 命令重置特定配置节

**Q: 鼠标模式点击不准确？**
- Android 鼠标加速会影响 HID 鼠标定位精度
- 可通过 `CALIBRATE` 命令调整灵敏度和移动延迟
- 建议优先使用键盘模式完成操作