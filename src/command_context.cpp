// BlueHIDFlow 命令执行上下文实现
#include "command_context.h"
#include "ble_hid.h"
#include "config_manager.h"
#include "mqtt_client.h"

bool CommandContext::parseParams(JsonDocument& doc) const {
    if (!params || strlen(params) == 0) {
        return false;
    }
    DeserializationError error = deserializeJson(doc, params);
    if (error != DeserializationError::Ok) {
        Serial.printf("CMD: JSON 解析错误: %s\n", error.c_str());
        return false;
    }
    return true;
}

void CommandContext::ensureScreenConfig() const {
    if (hid.getScreenWidth() != config.phone().screenWidth ||
        hid.getScreenHeight() != config.phone().screenHeight) {
        hid.setScreenSize(
            config.phone().screenWidth,
            config.phone().screenHeight
        );
        Serial.printf("CMD: 屏幕配置已更新: %d x %d\n",
                      config.phone().screenWidth,
                      config.phone().screenHeight);
    }
    if (hid.getHIDSensitivity() != config.phone().hidSensitivity) {
        hid.setHIDSensitivity(config.phone().hidSensitivity);
    }
    if (hid.getMoveDelay() != config.phone().moveDelayMs) {
        hid.setMoveDelay(config.phone().moveDelayMs);
    }
}