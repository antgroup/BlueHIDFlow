// BlueHIDFlow 键盘命令实现
// INPUT, KEY_PRESS, KEY_COMBO, HOME, BACK, VOLUME_UP, VOLUME_DOWN
#include "command_registry.h"
#include "command_context.h"
#include "ble_hid.h"
#include "config_manager.h"
#include "mqtt_client.h"

// ============================================================
// 按键码字符串映射
// ============================================================
static uint16_t parseKeyCode(const char* keyStr) {
    if (!keyStr) return (uint16_t)KeyCode::ENTER;

    // 导航键
    if (strcmp(keyStr, "HOME") == 0)      return (uint16_t)KeyCode::HOME;
    if (strcmp(keyStr, "BACK") == 0)       return (uint16_t)KeyCode::BACK;
    if (strcmp(keyStr, "ENTER") == 0)      return (uint16_t)KeyCode::ENTER;
    if (strcmp(keyStr, "ESCAPE") == 0 || strcmp(keyStr, "ESC") == 0)
                                            return (uint16_t)KeyCode::ESCAPE;
    if (strcmp(keyStr, "BACKSPACE") == 0)  return (uint16_t)KeyCode::BACKSPACE;
    if (strcmp(keyStr, "DELETE") == 0)     return (uint16_t)KeyCode::DELETE_KEY;
    if (strcmp(keyStr, "TAB") == 0)        return (uint16_t)KeyCode::TAB;
    if (strcmp(keyStr, "SPACE") == 0)      return (uint16_t)KeyCode::SPACE;

    // 方向键
    if (strcmp(keyStr, "UP") == 0)         return (uint16_t)KeyCode::UP;
    if (strcmp(keyStr, "DOWN") == 0)       return (uint16_t)KeyCode::DOWN;
    if (strcmp(keyStr, "LEFT") == 0)       return (uint16_t)KeyCode::LEFT;
    if (strcmp(keyStr, "RIGHT") == 0)      return (uint16_t)KeyCode::RIGHT;

    // 翻页
    if (strcmp(keyStr, "PAGE_UP") == 0)    return (uint16_t)KeyCode::PAGE_UP;
    if (strcmp(keyStr, "PAGE_DOWN") == 0)  return (uint16_t)KeyCode::PAGE_DOWN;

    // 功能键
    if (strcmp(keyStr, "F1") == 0)  return (uint16_t)KeyCode::F1;
    if (strcmp(keyStr, "F2") == 0)  return (uint16_t)KeyCode::F2;
    if (strcmp(keyStr, "F3") == 0)  return (uint16_t)KeyCode::F3;
    if (strcmp(keyStr, "F4") == 0)  return (uint16_t)KeyCode::F4;
    if (strcmp(keyStr, "F5") == 0)  return (uint16_t)KeyCode::F5;
    if (strcmp(keyStr, "F6") == 0)  return (uint16_t)KeyCode::F6;
    if (strcmp(keyStr, "F7") == 0)  return (uint16_t)KeyCode::F7;
    if (strcmp(keyStr, "F8") == 0)  return (uint16_t)KeyCode::F8;
    if (strcmp(keyStr, "F9") == 0)  return (uint16_t)KeyCode::F9;
    if (strcmp(keyStr, "F10") == 0) return (uint16_t)KeyCode::F10;
    if (strcmp(keyStr, "F11") == 0) return (uint16_t)KeyCode::F11;
    if (strcmp(keyStr, "F12") == 0) return (uint16_t)KeyCode::F12;

    // 媒体键
    if (strcmp(keyStr, "VOLUME_UP") == 0)       return (uint16_t)KeyCode::VOLUME_UP;
    if (strcmp(keyStr, "VOLUME_DOWN") == 0)     return (uint16_t)KeyCode::VOLUME_DOWN;
    if (strcmp(keyStr, "MUTE") == 0)            return (uint16_t)KeyCode::MUTE;
    if (strcmp(keyStr, "PLAY_PAUSE") == 0)      return (uint16_t)KeyCode::PLAY_PAUSE;
    if (strcmp(keyStr, "NEXT_TRACK") == 0)      return (uint16_t)KeyCode::NEXT_TRACK;
    if (strcmp(keyStr, "PREV_TRACK") == 0)      return (uint16_t)KeyCode::PREV_TRACK;

    // 浏览器键
    if (strcmp(keyStr, "BROWSER_BACK") == 0)    return (uint16_t)KeyCode::BROWSER_BACK;
    if (strcmp(keyStr, "BROWSER_HOME") == 0)    return (uint16_t)KeyCode::BROWSER_HOME;

    // 焦点激活键
    if (strcmp(keyStr, "DPAD_CENTER") == 0)   return (uint16_t)KeyCode::DPAD_CENTER;

    // 单字符按键（字母/数字）
    char c = keyStr[0];
    if (strlen(keyStr) == 1 && c >= 'A' && c <= 'Z') {
        return 0x0100 | (0x04 + (c - 'A'));
    }
    if (strlen(keyStr) == 1 && c >= 'a' && c <= 'z') {
        return 0x0100 | (0x04 + (c - 'a'));
    }
    if (strlen(keyStr) == 1 && c >= '1' && c <= '9') {
        return 0x0100 | (0x1E + (c - '1'));
    }
    if (strlen(keyStr) == 1 && c == '0') {
        return 0x0100 | 0x27;
    }

    return 0;  // 未知按键
}

// 修饰键字符串解析
static uint8_t parseModifiers(const JsonArray& modifiers) {
    uint8_t mod = 0;
    for (JsonVariant m : modifiers) {
        const char* modStr = m.as<const char*>();
        if (strcmp(modStr, "ctrl") == 0 || strcmp(modStr, "CTRL") == 0)
            mod |= (uint8_t)KeyModifier::CTRL;
        else if (strcmp(modStr, "shift") == 0 || strcmp(modStr, "SHIFT") == 0)
            mod |= (uint8_t)KeyModifier::SHIFT;
        else if (strcmp(modStr, "alt") == 0 || strcmp(modStr, "ALT") == 0)
            mod |= (uint8_t)KeyModifier::ALT;
        else if (strcmp(modStr, "gui") == 0 || strcmp(modStr, "GUI") == 0)
            mod |= (uint8_t)KeyModifier::GUI;
    }
    return mod;
}

// ============================================================
// INPUT 命令 - 文本输入
// ============================================================
class InputCommand : public ICommand {
public:
    const char* name() const override { return "INPUT"; }
    CommandResult execute(CommandContext& ctx) override {
        Serial.println("CMD: 执行 INPUT 命令");

        if (ctx.hid.getCurrentMode() != HIDMode::KEYBOARD) {
            Serial.println("CMD: 错误 - INPUT 命令需要键盘模式，请使用 SWITCH_MODE 切换");
            return FAILED;
        }

        if (!ctx.params || strlen(ctx.params) == 0) {
            Serial.println("CMD: 错误 - params 为空");
            return FAILED;
        }

        JsonDocument doc;
        if (!ctx.parseParams(doc)) return FAILED;

        JsonObject paramsObj = doc.as<JsonObject>();
        if (!paramsObj.containsKey("text")) {
            Serial.println("CMD: 错误 - input 命令需要 'text' 参数");
            return FAILED;
        }

        const char* text = paramsObj["text"];
        Serial.printf("CMD: 输入文本: %s\n", text);

        if (!ctx.hid.isConnected()) {
            Serial.println("CMD: 错误 - BLE 未连接");
            return FAILED;
        }

        bool result = ctx.hid.typeString(text);
        if (!result) {
            Serial.println("CMD: 文本输入失败");
            return FAILED;
        }

        // 可选确认键
        if (paramsObj.containsKey("confirm")) {
            const char* confirm = paramsObj["confirm"];
            delay(500);
            if (strcmp(confirm, "space") == 0) {
                ctx.hid.keyPress((uint16_t)KeyCode::SPACE);
                Serial.println("CMD: 按空格确认");
            } else if (strcmp(confirm, "enter") == 0) {
                ctx.hid.keyPress((uint16_t)KeyCode::ENTER);
                Serial.println("CMD: 按回车确认");
            }
        }

        Serial.println("CMD: 文本输入成功");
        return SUCCESS;
    }
};
REGISTER_COMMAND(InputCommand)

// ============================================================
// KEY_PRESS 命令 - 单键点击
// ============================================================
class KeyPressCommand : public ICommand {
public:
    const char* name() const override { return "KEY_PRESS"; }
    CommandResult execute(CommandContext& ctx) override {
        Serial.println("CMD: 执行 KEY_PRESS 命令");

        if (ctx.hid.getCurrentMode() != HIDMode::KEYBOARD) {
            Serial.println("CMD: 错误 - KEY_PRESS 命令需要键盘模式");
            return FAILED;
        }

        if (!ctx.params || strlen(ctx.params) == 0) {
            Serial.println("CMD: 错误 - params 为空");
            return FAILED;
        }

        JsonDocument doc;
        if (!ctx.parseParams(doc)) return FAILED;

        JsonObject paramsObj = doc.as<JsonObject>();
        if (!paramsObj.containsKey("key")) {
            Serial.println("CMD: 错误 - KEY_PRESS 命令需要 'key' 参数");
            return FAILED;
        }

        const char* keyStr = paramsObj["key"];
        Serial.printf("CMD: 按键: %s\n", keyStr);

        if (!ctx.hid.isConnected()) {
            Serial.println("CMD: 错误 - BLE 未连接");
            return FAILED;
        }

        // 单字符按键
        if (strlen(keyStr) == 1) {
            char c = keyStr[0];
            if (c >= 'A' && c <= 'Z') {
                char lower[2] = {(char)(c + 32), '\0'};
                return ctx.hid.typeString(lower) ? SUCCESS : FAILED;
            }
            if (c >= 'a' && c <= 'z') {
                char str[2] = {c, '\0'};
                return ctx.hid.typeString(str) ? SUCCESS : FAILED;
            }
            if (c >= '0' && c <= '9') {
                char str[2] = {c, '\0'};
                return ctx.hid.typeString(str) ? SUCCESS : FAILED;
            }
        }

        uint16_t keyCode = parseKeyCode(keyStr);
        if (keyCode == 0) {
            Serial.printf("CMD: 未知按键: %s\n", keyStr);
            return FAILED;
        }

        // 检查修饰键
        uint8_t modifiers = 0;
        if (paramsObj.containsKey("modifier")) {
            const char* modStr = paramsObj["modifier"];
            if (paramsObj["modifier"].is<const char*>()) {
                if (strcmp(modStr, "ctrl") == 0 || strcmp(modStr, "CTRL") == 0)
                    modifiers = (uint8_t)KeyModifier::CTRL;
                else if (strcmp(modStr, "shift") == 0 || strcmp(modStr, "SHIFT") == 0)
                    modifiers = (uint8_t)KeyModifier::SHIFT;
                else if (strcmp(modStr, "alt") == 0 || strcmp(modStr, "ALT") == 0)
                    modifiers = (uint8_t)KeyModifier::ALT;
                else if (strcmp(modStr, "gui") == 0 || strcmp(modStr, "GUI") == 0)
                    modifiers = (uint8_t)KeyModifier::GUI;
            } else if (paramsObj["modifier"].is<JsonArray>()) {
                modifiers = parseModifiers(paramsObj["modifier"].as<JsonArray>());
            }
        }

        if (modifiers != 0) {
            return ctx.hid.keyCombo(keyCode, modifiers) ? SUCCESS : FAILED;
        }

        return ctx.hid.keyPress(keyCode) ? SUCCESS : FAILED;
    }
};
REGISTER_COMMAND(KeyPressCommand)

// ============================================================
// KEY_COMBO 命令 - 组合键
// ============================================================
class KeyComboCommand : public ICommand {
public:
    const char* name() const override { return "KEY_COMBO"; }
    CommandResult execute(CommandContext& ctx) override {
        Serial.println("CMD: 执行 KEY_COMBO 命令");

        if (ctx.hid.getCurrentMode() != HIDMode::KEYBOARD) {
            Serial.println("CMD: 错误 - KEY_COMBO 命令需要键盘模式");
            return FAILED;
        }

        if (!ctx.params || strlen(ctx.params) == 0) {
            Serial.println("CMD: 错误 - params 为空");
            return FAILED;
        }

        JsonDocument doc;
        if (!ctx.parseParams(doc)) return FAILED;

        JsonObject paramsObj = doc.as<JsonObject>();
        if (!paramsObj.containsKey("key") || !paramsObj.containsKey("modifiers")) {
            Serial.println("CMD: 错误 - KEY_COMBO 命令需要 'key' 和 'modifiers' 参数");
            return FAILED;
        }

        const char* keyStr = paramsObj["key"];
        uint8_t modifiers = parseModifiers(paramsObj["modifiers"].as<JsonArray>());

        Serial.printf("CMD: 组合键: %s + 修饰键(0x%02X)\n", keyStr, modifiers);

        if (!ctx.hid.isConnected()) {
            Serial.println("CMD: 错误 - BLE 未连接");
            return FAILED;
        }

        uint16_t keyCode = parseKeyCode(keyStr);
        if (keyCode == 0) {
            Serial.printf("CMD: 未知按键: %s\n", keyStr);
            return FAILED;
        }

        return ctx.hid.keyCombo(keyCode, modifiers) ? SUCCESS : FAILED;
    }
};
REGISTER_COMMAND(KeyComboCommand)

// ============================================================
// HOME 命令
// ============================================================
class HomeCommand : public ICommand {
public:
    const char* name() const override { return "HOME"; }
    CommandResult execute(CommandContext& ctx) override {
        Serial.println("CMD: 执行 HOME 命令");
        if (ctx.hid.getCurrentMode() != HIDMode::KEYBOARD) {
            Serial.println("CMD: 错误 - HOME 命令需要键盘模式");
            return FAILED;
        }
        if (!ctx.hid.isConnected()) {
            Serial.println("CMD: 错误 - BLE 未连接");
            return FAILED;
        }
        return ctx.hid.keyPress((uint16_t)KeyCode::HOME) ? SUCCESS : FAILED;
    }
};
REGISTER_COMMAND(HomeCommand)

// ============================================================
// BACK 命令
// ============================================================
class BackCommand : public ICommand {
public:
    const char* name() const override { return "BACK"; }
    CommandResult execute(CommandContext& ctx) override {
        Serial.println("CMD: 执行 BACK 命令");
        if (ctx.hid.getCurrentMode() != HIDMode::KEYBOARD) {
            Serial.println("CMD: 错误 - BACK 命令需要键盘模式");
            return FAILED;
        }
        if (!ctx.hid.isConnected()) {
            Serial.println("CMD: 错误 - BLE 未连接");
            return FAILED;
        }
        return ctx.hid.keyPress((uint16_t)KeyCode::BACK) ? SUCCESS : FAILED;
    }
};
REGISTER_COMMAND(BackCommand)

// ============================================================
// VOLUME_UP 命令
// ============================================================
class VolumeUpCommand : public ICommand {
public:
    const char* name() const override { return "VOLUME_UP"; }
    CommandResult execute(CommandContext& ctx) override {
        Serial.println("CMD: 执行 VOLUME_UP 命令");
        if (ctx.hid.getCurrentMode() != HIDMode::KEYBOARD) {
            Serial.println("CMD: 错误 - VOLUME_UP 命令需要键盘模式");
            return FAILED;
        }
        if (!ctx.hid.isConnected()) {
            Serial.println("CMD: 错误 - BLE 未连接");
            return FAILED;
        }
        return ctx.hid.keyPress((uint16_t)KeyCode::VOLUME_UP) ? SUCCESS : FAILED;
    }
};
REGISTER_COMMAND(VolumeUpCommand)

// ============================================================
// VOLUME_DOWN 命令
// ============================================================
class VolumeDownCommand : public ICommand {
public:
    const char* name() const override { return "VOLUME_DOWN"; }
    CommandResult execute(CommandContext& ctx) override {
        Serial.println("CMD: 执行 VOLUME_DOWN 命令");
        if (ctx.hid.getCurrentMode() != HIDMode::KEYBOARD) {
            Serial.println("CMD: 错误 - VOLUME_DOWN 命令需要键盘模式");
            return FAILED;
        }
        if (!ctx.hid.isConnected()) {
            Serial.println("CMD: 错误 - BLE 未连接");
            return FAILED;
        }
        return ctx.hid.keyPress((uint16_t)KeyCode::VOLUME_DOWN) ? SUCCESS : FAILED;
    }
};
REGISTER_COMMAND(VolumeDownCommand)