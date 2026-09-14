# BlueHIDFlow 硬件接线与调试指南

**硬件清单**：
- ESP32-S3-N16R8 主控制器 x1
- OV2640 摄像头模块 x1（可选，当前固件中已禁用）
- RGB LED 指示灯 x1
- 蜂鸣器 x1（可选）

---

## ESP32-S3 主控制器接线（基础）

### USB 连接

| 引脚 | 功能 | 说明 |
|------|--------|--------|
| USB D+ / D- | USB 串口数据 | 用于烧录固件和调试串口输出 |
| USB 5V / 5V | USB 电源 | 为开发板供电 |

**调试准备**：连接 USB 到电脑，使用 `pio device monitor` 或串口工具查看调试输出

---

## OV2640 摄像头模块接线（可选）

OV2640 通过 DVP 并行接口连接到 ESP32-S3，引脚定义如下（详见 `include/gpio_config.h`）：

| OV2640 | ESP32-S3 | 说明 |
|--------|----------|------|
| XCLK | GPIO10 | 系统时钟 |
| SIOD | GPIO40 | SCCB SDA |
| SIOC | GPIO39 | SCCB SCL |
| D0-D7 | GPIO15-18, GPIO12-14, GPIO16, GPIO48 | 数据总线 |
| VSYNC | GPIO38 | 垂直同步 |
| HREF | GPIO47 | 水平参考 |
| PCLK | GPIO13 | 像素时钟 |
| VCC | 3.3V | 电源 |
| GND | GND | 地线 |

> 具体引脚映射请查阅 `include/gpio_config.h` 中的 `CAM_*` 宏定义。

---

## LED 状态指示灯

使用共阴 RGB LED，低电平点亮：

| LED 颜色 | GPIO | 功能 |
|----------|------|------|
| 红色 | GPIO3 | 错误/异常 / MQTT 断开 |
| 绿色 | GPIO4 | 正常运行 |
| 蓝色 | GPIO5 | 设备忙 |

### 接线方式

- **R/G/B 引脚** → 分别接 GPIO3/4/5（通过 220Ω 限流电阻）
- **共阴极** → GND

---

## 蜂鸣器（可选）

| 蜂鸣器端 | ESP32-S3 | 说明 |
|----------|-----------|------|
| 正极 (+) | GPIO8 | PWM 输出 |
| 负极 (-) | GND | 地线 |

使用 `LEDC` PWM 驱动产生蜂鸣声，代码中通过 `tone()` / `noTone()` 或 `ledcWrite()` 控制。

---

## ESP32-S3 主控制器基础测试

### 1. 验证固件烧录

```bash
pio run --target upload
```

**预期结果**：固件烧录成功，串口输出 `BlueHIDFlow Firmware v0.9.0`

### 2. WiFi 连接测试

固件启动后会尝试连接到 MQTT broker，需要确认：

1. WiFi SSID 和密码正确配置（在 Web 配置页面或 `include/config.h` 中）
2. MQTT broker 地址可访问
3. 网络可达性测试

```bash
# 查看串口输出
pio device monitor
```

**预期输出**：
```
[Main] WiFi 已连接
[Main] IP: 192.168.x.x
[Main] MQTT 已连接
```

### 3. MQTT 通信测试

使用 MQTTX 或其他 MQTT 客户端测试连接：

| 订阅主题 | 预期消息 |
|----------|----------|
| `bluehidflow/status/{device_id}` | 在线状态 |
| `bluehidflow/response/{device_id}` | 命令执行结果 |

**测试命令**：
```json
{"action": "STATUS", "message_id": "test-001"}
```

> `{device_id}` 为设备自动生成的 ID（基于 MAC 地址），上电后可在串口日志或 Web 配置页面查看。

---

## 调试步骤

### 开发环境准备

```bash
# 1. 安装 PlatformIO（如果未安装）
pip install platformio

# 2. 克隆仓库并安装依赖
git clone git@github.com:antgroup/BlueHIDFlow.git
cd BlueHIDFlow
pio pkg install

# 3. 打开源码
code .
```

### 初始 Flash 测试

1. **烧录固件**：
```bash
pio run --target upload
```

2. **验证串口输出**：
```bash
pio device monitor
```

**预期**：看到 `BlueHIDFlow Firmware v0.9.0` 和 WiFi/MQTT 连接日志

### 功能分步测试

#### 第 1 步：主控基础功能
- [ ] 串口通信正常
- [ ] WiFi 连接功能
- [ ] MQTT broker 连接
- [ ] 设备状态上报
- [ ] LED 指示灯控制

#### 第 2 步：BLE HID 功能
- [ ] BLE 广播可见
- [ ] 手机连接成功
- [ ] TAP / SWIPE 命令执行准确
- [ ] 键盘输入 / 按键命令正常

---

## 常见问题诊断

### 问题 1：固件烧录失败

**症状**：`pio run --target upload` 报错

**检查**：
```bash
# 检查 USB 连接
ls /dev/cu.usb*

# 检查串口权限
ls -la /dev/cu.u*
```

### 问题 2：串口无输出

**可能原因**：
1. 烧录了旧固件但串口波特率不匹配
2. 使用了错误的串口工具

**解决**：
```bash
# 常用波特率
screen /dev/cu.usbmodem* 115200
```

### 问题 3：WiFi 连接失败

**症状**：串口显示 WiFi 连接失败

**检查清单**：
- WiFi SSID 和密码是否正确
- 路由器是否正常运行
- ESP32-S3 WiFi 范围是否足够

### 问题 4：MQTT 无法连接

**症状**：连接失败或频繁断开

**检查清单**：
- MQTT broker 地址是否正确
- MQTT 用户名/密码配置是否正确
- TLS CA 证书是否已配置（端口 8883 需要）

**调试方法**：
```bash
# 使用 MQTTX 测试连接
# Broker: your-mqtt-broker.example.com:8883
# Port: 8883
# Username: <your-mqtt-username>
# Password: <your-mqtt-password>
# Client ID: 设备 ID
```

---

## 调试技巧

### 1. 基础 LED 闪烁测试

```cpp
void setup() {
    Serial.begin(115200);
    pinMode(LED_R_PIN, OUTPUT);
    pinMode(LED_G_PIN, OUTPUT);
    pinMode(LED_B_PIN, OUTPUT);
}

void loop() {
    static uint8_t idx = 0;
    const uint8_t pins[] = {LED_R_PIN, LED_G_PIN, LED_B_PIN};
    for (uint8_t i = 0; i < 3; i++) {
        digitalWrite(pins[i], i == idx ? LOW : HIGH);
    }
    idx = (idx + 1) % 3;
    delay(500);
}
```

### 2. WiFi 状态监控

```cpp
void printWiFiStatus() {
    Serial.printf("WiFi: %s, RSSI: %d\r\n",
        WiFi.isConnected() ? "Connected" : "Disconnected",
        WiFi.RSSI());
}
```

### 3. 内存使用监控

```cpp
Serial.printf("Free heap: %u bytes\r\n", ESP.getFreeHeap());
Serial.printf("Min free heap: %u bytes\r\n", ESP.getMinFreeHeap());
```

---

## 测试完成标准

每个功能测试完成后，确认以下内容：

- [ ] 功能在预期范围内正常工作
- [ ] 串口输出稳定无错误信息
- [ ] LED 指示正确反映设备状态
- [ ] MQTT 消息正确发送和接收
- [ ] BLE HID 点击/滑动准确

---

**创建日期**: 2026-04-14
**适用场景**: BlueHIDFlow 硬件集成测试
**文档版本**: v1.2

如有任何问题或需要进一步的指导，请提供：
1. 当前硬件接线的照片
2. 串口输出的完整日志
3. 使用的手机型号和系统版本
