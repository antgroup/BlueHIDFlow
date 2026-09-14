// BlueHIDFlow MQTT 发布接口
// 解耦 command_handler 对 MqttClient 的直接依赖
#ifndef IMQTT_PUBLISHER_H
#define IMQTT_PUBLISHER_H

#include <Arduino.h>
#include <ArduinoJson.h>

// ============================================================
// MQTT 发布接口
// ============================================================
class IMqttPublisher {
public:
    virtual ~IMqttPublisher() = default;

    // 发布原始消息
    virtual bool publish(const char* topic, const char* payload, bool retained = false) = 0;

    // 发布命令响应（简化接口）
    virtual bool publishResponse(const char* messageId, const char* status, const char* message = nullptr) = 0;

    // 发布带数据的命令响应
    virtual bool publishResponseWithData(const char* messageId, const char* status, JsonDocument& data) = 0;

    // 连接状态
    virtual bool isConnected() = 0;
};

#endif // IMQTT_PUBLISHER_H