// BlueHIDFlow 鼠标命令实现
// TAP, SWIPE, SWIPE_UP/DOWN/LEFT/RIGHT, CALIBRATE
#include "command_registry.h"
#include "command_context.h"
#include "ble_hid.h"
#include "config_manager.h"
#include "mqtt_client.h"

// ============================================================
// TAP 命令 - 绝对坐标点击
// ============================================================
class TapCommand : public ICommand {
public:
    const char* name() const override { return "TAP"; }
    CommandResult execute(CommandContext& ctx) override {
        Serial.println("CMD: ===== 执行 TAP 命令 =====");

        if (ctx.hid.getCurrentMode() != HIDMode::MOUSE) {
            Serial.println("CMD: 错误 - TAP 命令需要鼠标模式，请使用 SWITCH_MODE 切换");
            return FAILED;
        }

        ctx.ensureScreenConfig();

        if (!ctx.params || strlen(ctx.params) == 0) {
            Serial.println("CMD: 错误 - params 为空");
            return FAILED;
        }

        Serial.printf("CMD: 原始参数: %s\n", ctx.params);

        JsonDocument doc;
        if (!ctx.parseParams(doc)) return FAILED;

        JsonObject paramsObj = doc.as<JsonObject>();
        if (!paramsObj.containsKey("x") || !paramsObj.containsKey("y")) {
            Serial.println("CMD: 错误 - 缺少必要参数 (x, y)");
            return FAILED;
        }

        int x = paramsObj["x"];
        int y = paramsObj["y"];
        int duration = paramsObj.containsKey("duration") ? paramsObj["duration"].as<int>() : 50;

        uint16_t maxX = ctx.config.phone().screenWidth;
        uint16_t maxY = ctx.config.phone().screenHeight;
        if (x < 0 || x > maxX || y < 0 || y > maxY) {
            Serial.printf("CMD: 警告 - 坐标 (%d, %d) 超出屏幕范围 (%d, %d)，将自动限制\n",
                          x, y, maxX, maxY);
        }

        Serial.printf("CMD: 点击坐标: (%d, %d), duration=%dms\n", x, y, duration);

        if (ctx.hid.isConnected()) {
            Serial.println("CMD: BLE 已连接，执行绝对坐标点击...");
            bool result = ctx.hid.tapAbsolute((uint16_t)x, (uint16_t)y, duration);
            if (result) {
                Serial.println("CMD: 点击命令执行成功");
            } else {
                Serial.println("CMD: 点击命令执行失败");
            }
            return result ? SUCCESS : FAILED;
        } else {
            Serial.println("CMD: 错误 - BLE 未连接");
            return FAILED;
        }
    }
};
REGISTER_COMMAND(TapCommand)

// ============================================================
// SWIPE 命令 - 绝对坐标滑动
// ============================================================
class SwipeCommand : public ICommand {
public:
    const char* name() const override { return "SWIPE"; }
    CommandResult execute(CommandContext& ctx) override {
        Serial.println("CMD: ===== 执行 SWIPE 命令 =====");

        if (ctx.hid.getCurrentMode() != HIDMode::MOUSE) {
            Serial.println("CMD: 错误 - SWIPE 命令需要鼠标模式，请使用 SWITCH_MODE 切换");
            return FAILED;
        }

        ctx.ensureScreenConfig();

        if (!ctx.params || strlen(ctx.params) == 0) {
            Serial.println("CMD: 错误 - params 为空");
            return FAILED;
        }

        Serial.printf("CMD: 原始参数: %s\n", ctx.params);

        JsonDocument doc;
        if (!ctx.parseParams(doc)) return FAILED;

        JsonObject paramsObj = doc.as<JsonObject>();
        if (!paramsObj.containsKey("from_x") || !paramsObj.containsKey("from_y") ||
            !paramsObj.containsKey("to_x") || !paramsObj.containsKey("to_y")) {
            Serial.println("CMD: 错误 - 缺少必要参数 (from_x, from_y, to_x, to_y)");
            return FAILED;
        }

        int from_x = paramsObj["from_x"];
        int from_y = paramsObj["from_y"];
        int to_x = paramsObj["to_x"];
        int to_y = paramsObj["to_y"];
        int duration = paramsObj.containsKey("duration") ? paramsObj["duration"].as<int>() : 300;

        Serial.printf("CMD: 滑动坐标: (%d,%d) -> (%d,%d), duration=%dms\n",
                      from_x, from_y, to_x, to_y, duration);

        if (ctx.hid.isConnected()) {
            Serial.println("CMD: BLE 已连接，执行绝对坐标滑动...");
            bool result = ctx.hid.swipeAbsolute(
                (uint16_t)from_x, (uint16_t)from_y,
                (uint16_t)to_x, (uint16_t)to_y,
                duration
            );
            if (result) {
                Serial.println("CMD: 滑动命令执行成功");
            } else {
                Serial.println("CMD: 滑动命令执行失败");
            }
            return result ? SUCCESS : FAILED;
        } else {
            Serial.println("CMD: 错误 - BLE 未连接");
            return FAILED;
        }
    }
};
REGISTER_COMMAND(SwipeCommand)

// ============================================================
// 方向滑动命令 - SWIPE_UP/DOWN/LEFT/RIGHT
// ============================================================
class SwipeDirectionCommand : public ICommand {
private:
    SwipeDirection _direction;
    const char* _nameStr;
    const char* _dirName;

public:
    SwipeDirectionCommand(SwipeDirection dir, const char* nameStr, const char* dirName)
        : _direction(dir), _nameStr(nameStr), _dirName(dirName) {}

    const char* name() const override { return _nameStr; }

    CommandResult execute(CommandContext& ctx) override {
        Serial.println("CMD: ===== 执行方向滑动命令 =====");

        if (ctx.hid.getCurrentMode() != HIDMode::MOUSE) {
            Serial.println("CMD: 错误 - 方向滑动命令需要鼠标模式，请使用 SWITCH_MODE 切换");
            return FAILED;
        }

        ctx.ensureScreenConfig();

        int duration = 300;
        if (ctx.params && strlen(ctx.params) > 0) {
            JsonDocument doc;
            if (ctx.parseParams(doc)) {
                JsonObject paramsObj = doc.as<JsonObject>();
                if (paramsObj.containsKey("duration")) {
                    duration = paramsObj["duration"];
                }
            }
        }

        Serial.printf("CMD: 方向=%s, duration=%dms\n", _dirName, duration);

        if (ctx.hid.isConnected()) {
            Serial.println("CMD: BLE 已连接，执行预设方向滑动...");
            bool result = ctx.hid.swipeDirection(_direction, duration);
            if (result) {
                Serial.println("CMD: 方向滑动命令执行成功");
            } else {
                Serial.println("CMD: 方向滑动命令执行失败");
            }
            return result ? SUCCESS : FAILED;
        } else {
            Serial.println("CMD: 错误 - BLE 未连接");
            return FAILED;
        }
    }
};

// 手动注册方向滑动命令（带构造参数）
static SwipeDirectionCommand _cmd_swipe_up(SwipeDirection::UP, "SWIPE_UP", "向上");
static SwipeDirectionCommand _cmd_swipe_down(SwipeDirection::DOWN, "SWIPE_DOWN", "向下");
static SwipeDirectionCommand _cmd_swipe_left(SwipeDirection::LEFT, "SWIPE_LEFT", "向左");
static SwipeDirectionCommand _cmd_swipe_right(SwipeDirection::RIGHT, "SWIPE_RIGHT", "向右");
static bool _reg_swipe_dir = [](){
    CommandRegistry::instance().registerCommand(&_cmd_swipe_up);
    CommandRegistry::instance().registerCommand(&_cmd_swipe_down);
    CommandRegistry::instance().registerCommand(&_cmd_swipe_left);
    CommandRegistry::instance().registerCommand(&_cmd_swipe_right);
    return true;
}();

// ============================================================
// CALIBRATE 命令 - 校准
// ============================================================
class CalibrateCommand : public ICommand {
public:
    const char* name() const override { return "CALIBRATE"; }
    CommandResult execute(CommandContext& ctx) override {
        Serial.println("CMD: ===== 执行校准命令 =====");

        if (ctx.hid.getCurrentMode() != HIDMode::MOUSE) {
            Serial.println("CMD: 错误 - CALIBRATE 命令需要鼠标模式，请使用 SWITCH_MODE 切换");
            return FAILED;
        }

        if (!ctx.params || strlen(ctx.params) == 0) {
            Serial.println("CMD: 错误 - params 为空");
            return FAILED;
        }

        Serial.printf("CMD: 原始参数: %s\n", ctx.params);

        JsonDocument doc;
        if (!ctx.parseParams(doc)) return FAILED;

        JsonObject paramsObj = doc.as<JsonObject>();

        if (ctx.hid.isConnected()) {
            if (paramsObj.containsKey("sensitivity")) {
                float sensitivity = paramsObj["sensitivity"];
                ctx.hid.setHIDSensitivity(sensitivity);
                ctx.config.phone().hidSensitivity = sensitivity;
                ctx.config.save();
                Serial.printf("CMD: 灵敏度已设置为 %.2f 像素/HID单位并保存\n", sensitivity);
                return SUCCESS;
            }
            else if (paramsObj.containsKey("move_delay")) {
                uint16_t delayMs = paramsObj["move_delay"];
                ctx.hid.setMoveDelay(delayMs);
                ctx.config.phone().moveDelayMs = delayMs;
                ctx.config.save();
                Serial.printf("CMD: 移动延迟已设置为 %dms 并保存\n", delayMs);
                return SUCCESS;
            }
            else if (paramsObj.containsKey("hid_x") && paramsObj.containsKey("hid_y")) {
                int hidX = paramsObj["hid_x"];
                int hidY = paramsObj["hid_y"];
                int delayMs = paramsObj.containsKey("delay_ms") ? paramsObj["delay_ms"].as<int>() : 50;
                bool stepByStep = paramsObj.containsKey("step_by_step") ? paramsObj["step_by_step"].as<bool>() : false;
                Serial.printf("CMD: 执行校准滑动 HID: (%d, %d), 每步延迟: %dms, 逐步模式: %s\n",
                              hidX, hidY, delayMs, stepByStep ? "是" : "否");
                return ctx.hid.calibrateMove(hidX, hidY, delayMs, stepByStep) ? SUCCESS : FAILED;
            }
            else {
                Serial.println("CMD: 错误 - 需要参数 'sensitivity'、'move_delay' 或 'hid_x'/'hid_y'");
                return FAILED;
            }
        } else {
            Serial.println("CMD: 错误 - BLE 未连接");
            return FAILED;
        }
    }
};
REGISTER_COMMAND(CalibrateCommand)