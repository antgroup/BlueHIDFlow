// BlueHIDFlow 命令执行上下文
// 封装命令执行时的依赖注入和参数访问
#ifndef COMMAND_CONTEXT_H
#define COMMAND_CONTEXT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "imqtt_publisher.h"

// 前向声明
class BLEHIDDevice;
class ConfigManager;
class LogManager;

// ============================================================
// 命令执行上下文
// ============================================================
class CommandContext {
public:
    const char* action;
    const char* params;       // 原始 JSON 参数字符串
    const char* messageId;    // 消息 ID（用于 MQTT 响应关联）

    BLEHIDDevice& hid;
    IMqttPublisher& mqtt;     // 使用接口引用，解耦对 MqttClient 的直接依赖
    ConfigManager& config;
    LogManager& log;

    CommandContext(BLEHIDDevice& hid, IMqttPublisher& mqtt,
                   ConfigManager& config, LogManager& log)
        : action(nullptr), params(nullptr), messageId(nullptr),
          hid(hid), mqtt(mqtt), config(config), log(log) {}

    // 解析 params 为 JsonDocument
    bool parseParams(JsonDocument& doc) const;

    // 确保屏幕配置同步到 BLE HID（鼠标命令使用）
    void ensureScreenConfig() const;
};

#endif // COMMAND_CONTEXT_H