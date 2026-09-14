// BlueHIDFlow Command Handler
// 支持版本化命令协议，向后兼容旧格式
#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <Arduino.h>
#include "command_registry.h"
#include "command_context.h"
#include "mqtt_client.h"

// ============================================================
// 命令处理器类
// ============================================================
class CommandHandler {
private:
    bool _initialized = false;
    char _lastMessageId[64];
    CommandResult _lastResult;

public:
    CommandHandler() : _initialized(false), _lastResult(SUCCESS) {
        _lastMessageId[0] = '\0';
    }

    void begin();
    void loop();
    void execute(const char* action, const char* params);

    // 辅助函数
    const char* getLastMessageId() { return _lastMessageId; }
    void setLastMessageId(const char* id);
    void clearLastMessageId() { _lastMessageId[0] = '\0'; }
    CommandResult getLastResult() { return _lastResult; }
};

// 外部命令处理器实例
extern CommandHandler cmdHandler;

// 外部依赖实例（供 CommandContext 构造使用）
extern BLEHIDDevice bleHid;
extern MqttClient mqtt;       // MqttClient 继承 IMqttPublisher，可隐式转换
extern LogManager logManager;

// Message ID 管理辅助函数 (兼容旧接口)
void handleCommand(const char* payload);
const char* getLastMessageId();
void setLastMessageId(const char* id);
void clearLastMessageId();

#endif // COMMAND_HANDLER_H