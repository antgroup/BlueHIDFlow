// ============================================================
// BlueHIDFlow Firmware 设备配置示例
// ============================================================
//
// 使用方法：
// 1. 复制此文件为 config.h：cp config.example.h config.h
// 2. 根据实际环境修改配置值
// 3. config.h 不应提交到版本控制系统（已加入 .gitignore）
//
// 注意：所有配置项均可通过 Web 配置页面（AP 模式）运行时修改
// ============================================================

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include "gpio_config.h"

// ============================================
// WiFi 配置
// ============================================

// WiFi 网络 SSID（可通过 Web 配置页面覆盖）
#define WIFI_SSID "your_wifi_ssid"

// WiFi 网络密码
#define WIFI_PASSWORD "your_wifi_password"

// WiFi 连接超时时间（毫秒）
#define WIFI_CONNECT_TIMEOUT_MS 10000

// WiFi 重连延迟（毫秒）
#define WIFI_RECONNECT_DELAY_MS 5000

// ============================================
// 摄像头配置
// ============================================

// 摄像头分辨率（FRAMESIZE_SVGA = 800x600）
#define CAMERA_FRAME_SIZE FRAMESIZE_SVGA

// JPEG 质量 (1-31，值越小质量越高)
#define CAMERA_JPEG_QUALITY 12

// 帧缓冲数量
#define CAMERA_FB_COUNT 2

// ============================================
// BLE HID 配置
// ============================================

// BLE 设备名称（手机蓝牙搜索时显示的名称）
#define BLE_DEVICE_NAME "BlueHIDFlow-HID"

// BLE 广播超时（0 = 永久广播）
#define BLE_ADVERTISING_TIMEOUT 0

// ============================================
// 任务执行配置
// ============================================

// 任务最大重试次数
#define TASK_MAX_ATTEMPTS 10

// 任务最大执行时间（秒）
#define TASK_MAX_DURATION_SEC 120

// 动作间基础延迟（毫秒）
#define ACTION_DELAY_BASE_MS 500

// 验证间基础延迟（毫秒）
#define VERIFY_DELAY_BASE_MS 1000

// ============================================
// 坐标配置 (默认值，可通过 CALIBRATE 命令运行时修改)
// ============================================

// 默认屏幕宽度（像素）
#define DEFAULT_SCREEN_WIDTH 1080

// 默认屏幕高度（像素）
#define DEFAULT_SCREEN_HEIGHT 2400

// ============================================
// MQTT TLS CA 证书（PEM 格式）
// ============================================
// 若 MQTT Broker 使用 TLS（端口 8883），请粘贴 broker 提供的 CA 证书。
// 公共根 CA 证书本身不是秘密，但此处不内置任何厂商/服务专属证书，
// 避免项目与特定云服务商绑定。开发者按需自行配置。
#define MQTT_CA_CERT ""

// ============================================
// 调试配置
// ============================================

// 调试开关
#define DEBUG_ENABLED true
#define DEBUG_SERIAL Serial

#if DEBUG_ENABLED
    #define DEBUG_PRINT(...) DEBUG_SERIAL.printf(__VA_ARGS__)
    #define DEBUG_PRINTLN(...) DEBUG_SERIAL.println(__VA_ARGS__)
#else
    #define DEBUG_PRINT(...)
    #define DEBUG_PRINTLN(...)
#endif

#endif // CONFIG_H