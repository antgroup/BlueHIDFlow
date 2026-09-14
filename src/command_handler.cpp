// BlueHIDFlow Command Handler Implementation
// 使用命令注册表分发命令，支持版本化协议
#include "command_handler.h"
#include "ble_hid.h"
#include "config_manager.h"
#include "mqtt_client.h"
#include <ArduinoJson.h>

// 命令处理器实例
CommandHandler cmdHandler;

// ============================================================
// CommandHandler 类方法实现
// ============================================================

void CommandHandler::begin() {
    Serial.println("CMD: 命令处理器初始化完成");
    Serial.printf("CMD: 已注册 %d 个命令\n", CommandRegistry::instance().count());
    clearLastMessageId();
    _initialized = true;
}

void CommandHandler::loop() {
    // 空实现
}

void CommandHandler::execute(const char* action, const char* params) {
    Serial.printf("CMD: 执行动作: %s\n", action);

    // 创建命令上下文
    CommandContext ctx(bleHid, mqtt, configManager, logManager);
    ctx.action = action;
    ctx.params = params;
    ctx.messageId = _lastMessageId;

    // 在注册表中查找命令
    ICommand* cmd = CommandRegistry::instance().find(action);
    if (cmd) {
        _lastResult = cmd->execute(ctx);
    } else {
        Serial.printf("CMD: 未知动作：%s\n", action);
        _lastResult = INVALID_PARAMS;
    }

    Serial.printf("CMD: 命令结果: %d\n", _lastResult);

    // 统一发送 MQTT 响应（已自行发送响应的命令除外）
    if (_lastResult == SUCCESS) {
        mqtt.publishResponse(_lastMessageId, "SUCCESS", nullptr);
    } else if (_lastResult == FAILED) {
        mqtt.publishResponse(_lastMessageId, "FAILED", nullptr);
    }
}

void CommandHandler::setLastMessageId(const char* id) {
    if (id && strlen(id) < sizeof(_lastMessageId)) {
        strcpy(_lastMessageId, id);
        Serial.printf("CMD: 最后消息 ID 设置为: %s\n", id);
    }
}

// ============================================================
// 兼容旧接口的全局函数
// ============================================================

// 当前支持的协议版本
static const char* PROTOCOL_VERSION = "1.0";

void handleCommand(const char* payload) {
    Serial.printf("CMD: 收到负载：%s\n", payload);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error != DeserializationError::Ok) {
        Serial.printf("CMD: JSON 解析错误：%s\n", error.c_str());
        return;
    }

    JsonObject root = doc.as<JsonObject>();

    if (!root.containsKey("action")) {
        Serial.println("CMD: 错误 - 缺少 'action' 字段");
        return;
    }

    const char* action = root["action"];

    // 检测协议版本
    const char* version = "1.0";  // 默认版本
    const char* messageId = nullptr;
    const char* sessionId = nullptr;

    if (root.containsKey("version")) {
        // 新协议格式：包含 version 字段
        version = root["version"];
        Serial.printf("CMD: 协议版本：%s\n", version);

        // 新格式使用 "id" 字段
        if (root.containsKey("id")) {
            JsonVariant idVar = root["id"];
            if (idVar.is<const char*>()) {
                messageId = idVar.as<const char*>();
            } else if (idVar.is<unsigned long>()) {
                static char idBuf[32];
                snprintf(idBuf, sizeof(idBuf), "%lu", idVar.as<unsigned long>());
                messageId = idBuf;
            } else if (idVar.is<long>()) {
                static char idBuf[32];
                snprintf(idBuf, sizeof(idBuf), "%ld", idVar.as<long>());
                messageId = idBuf;
            }
        }

        // 新格式支持 metadata.sessionId
        if (root.containsKey("metadata")) {
            JsonObject metadata = root["metadata"].as<JsonObject>();
            if (metadata.containsKey("sessionId")) {
                sessionId = metadata["sessionId"];
                Serial.printf("CMD: 会话 ID：%s\n", sessionId);
            }
        }

        // 新格式支持 timestamp
        if (root.containsKey("timestamp")) {
            Serial.printf("CMD: 时间戳：%lu\n", (unsigned long)root["timestamp"]);
        }
    } else {
        // 旧协议格式：使用 message_id 字段
        version = "1.0";  // 旧格式默认为 1.0
        // message_id 由 mqtt_client.cpp 中的 callback 函数设置
        messageId = cmdHandler.getLastMessageId();
    }

    // 设置消息 ID（优先使用新格式的 id）
    if (messageId && strlen(messageId) > 0) {
        cmdHandler.setLastMessageId(messageId);
    } else {
        // 使用时间戳作为后备
        static char fallbackId[32];
        snprintf(fallbackId, sizeof(fallbackId), "%lu", millis());
        cmdHandler.setLastMessageId(fallbackId);
        messageId = cmdHandler.getLastMessageId();
    }

    Serial.printf("CMD: 动作：%s, 消息 ID: %s, 协议版本: %s\n", action, messageId, version);

    String paramsStr = "{}";
    if (root.containsKey("params")) {
        paramsStr = "";
        serializeJson(root["params"], paramsStr);
    }

    cmdHandler.execute(action, paramsStr.c_str());
}


const char* getLastMessageId() {
    return cmdHandler.getLastMessageId();
}

void setLastMessageId(const char* id) {
    cmdHandler.setLastMessageId(id);
}

void clearLastMessageId() {
    cmdHandler.clearLastMessageId();
}