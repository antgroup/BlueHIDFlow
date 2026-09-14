// BlueHIDFlow 系统命令实现
// WAIT, GET_STATUS, STATUS, RECONNECT, SWITCH_MODE
#include "command_registry.h"
#include "command_context.h"
#include "ble_hid.h"
#include "config_manager.h"
#include "mqtt_client.h"

// ============================================================
// WAIT 命令 - 等待指定时间
// ============================================================
class WaitCommand : public ICommand {
public:
    const char* name() const override { return "WAIT"; }
    CommandResult execute(CommandContext& ctx) override {
        Serial.println("CMD: 执行 WAIT 命令");

        if (!ctx.params || strlen(ctx.params) == 0) {
            Serial.println("CMD: 错误 - params 为空");
            return FAILED;
        }

        JsonDocument doc;
        if (!ctx.parseParams(doc)) return FAILED;

        JsonObject paramsObj = doc.as<JsonObject>();
        if (!paramsObj.containsKey("duration")) {
            Serial.println("CMD: 错误 - wait 命令需要 'duration' 参数");
            return FAILED;
        }

        int duration = paramsObj["duration"];
        Serial.printf("CMD: 等待 %d 毫秒...\n", duration);
        delay(duration);

        return SUCCESS;
    }
};
REGISTER_COMMAND(WaitCommand)

// ============================================================
// GET_STATUS 命令 - 获取设备状态（未实现）
// ============================================================
class GetStatusCommand : public ICommand {
public:
    const char* name() const override { return "GET_STATUS"; }
    CommandResult execute(CommandContext& ctx) override {
        Serial.println("CMD: 执行 GET_STATUS 命令");
        Serial.println("CMD: 设备状态:");
        return NOT_IMPLEMENTED;
    }
};
REGISTER_COMMAND(GetStatusCommand)

// ============================================================
// STATUS 命令 - 打印设备状态到串口
// ============================================================
class StatusCommand : public ICommand {
public:
    const char* name() const override { return "STATUS"; }
    CommandResult execute(CommandContext& ctx) override {
        Serial.println("CMD: 执行 STATUS 命令");
        Serial.println("CMD: ===== 设备状态 =====");
        Serial.printf("CMD: HID 模式: %s\n", ctx.hid.getModeName());
        Serial.printf("CMD: BLE 连接: %s\n", ctx.hid.isConnected() ? "已连接" : "未连接");
        Serial.printf("CMD: BLE 配对: %s\n", ctx.hid.isPaired() ? "已配对" : "未配对");
        Serial.printf("CMD: 屏幕尺寸: %d x %d\n", ctx.hid.getScreenWidth(), ctx.hid.getScreenHeight());
        Serial.printf("CMD: HID 灵敏度: %.2f 像素/HID单位\n", ctx.hid.getHIDSensitivity());
        Serial.printf("CMD: HID 移动延迟: %dms\n", ctx.hid.getMoveDelay());
        Serial.println("CMD: =====================");
        // STATUS 自行打印状态，不需要 MQTT 响应
        return RESPONSE_SENT;
    }
};
REGISTER_COMMAND(StatusCommand)

// ============================================================
// RECONNECT 命令 - 重连（未实现）
// ============================================================
class ReconnectCommand : public ICommand {
public:
    const char* name() const override { return "RECONNECT"; }
    CommandResult execute(CommandContext& ctx) override {
        Serial.println("CMD: 执行 RECONNECT 命令");
        Serial.println("CMD: 重连中...");
        return NOT_IMPLEMENTED;
    }
};
REGISTER_COMMAND(ReconnectCommand)

// ============================================================
// SWITCH_MODE 命令 - 切换 HID 模式
// ============================================================
class SwitchModeCommand : public ICommand {
public:
    const char* name() const override { return "SWITCH_MODE"; }
    CommandResult execute(CommandContext& ctx) override {
        Serial.println("CMD: 执行 SWITCH_MODE 命令");

        if (!ctx.params || strlen(ctx.params) == 0) {
            Serial.println("CMD: 错误 - params 为空");
            return FAILED;
        }

        JsonDocument doc;
        if (!ctx.parseParams(doc)) return FAILED;

        JsonObject paramsObj = doc.as<JsonObject>();
        if (!paramsObj.containsKey("mode")) {
            Serial.println("CMD: 错误 - SWITCH_MODE 命令需要 'mode' 参数");
            return FAILED;
        }

        const char* modeStr = paramsObj["mode"];
        HIDMode newMode;

        if (strcmp(modeStr, "keyboard") == 0 || strcmp(modeStr, "KEYBOARD") == 0) {
            newMode = HIDMode::KEYBOARD;
        } else if (strcmp(modeStr, "mouse") == 0 || strcmp(modeStr, "MOUSE") == 0) {
            newMode = HIDMode::MOUSE;
        } else {
            Serial.printf("CMD: 未知模式: %s (可选: keyboard, mouse)\n", modeStr);
            return FAILED;
        }

        Serial.printf("CMD: 切换模式到: %s\n", modeStr);

        // 发布响应（重启后将无法发送）
        JsonDocument dataDoc;
        dataDoc["newMode"] = modeStr;
        dataDoc["restarting"] = true;
        ctx.mqtt.publishResponseWithData(ctx.messageId, "SUCCESS", dataDoc);

        // 执行模式切换（会重启设备）
        ctx.hid.switchMode(newMode);
        return RESPONSE_SENT;
    }
};
REGISTER_COMMAND(SwitchModeCommand)