// BlueHIDFlow BLE HID - 统一键盘/鼠标模式实现
// v0.8.0 - 2026-04-21 重构版本
//
// KeyboardImpl 和 MouseImpl 分别在 ble_hid_keyboard.cpp 和 ble_hid_mouse.cpp 中定义
// 本文件不包含任何 HijelHID 头文件，避免两个库的枚举冲突
// 通过 IKeyboardImpl/IMouseImpl 接口指针访问库功能

#include "ble_hid.h"
#include "config_manager.h"

// ============================================
// 全局实例
// ============================================
BLEHIDDevice bleHid;

// ============================================
// 构造/析构
// ============================================
BLEHIDDevice::BLEHIDDevice() {}

BLEHIDDevice::~BLEHIDDevice() {
    if (_keyboard) { destroyKeyboardImpl(_keyboard); _keyboard = nullptr; }
    if (_mouse) { destroyMouseImpl(_mouse); _mouse = nullptr; }
}

// ============================================
// 生命周期
// ============================================
bool BLEHIDDevice::begin(const char* deviceName, HIDMode mode) {
    if (_initialized) {
        Serial.println("[BLE] 已初始化，跳过");
        return true;
    }

    _mode = mode;
    if (deviceName) {
        _deviceName = deviceName;
    }
    if (_deviceName.length() == 0) {
        _deviceName = "BlueHIDFlow-HID";
    }

    // 从配置读取屏幕参数
    _screenWidth = configManager.phone().screenWidth;
    _screenHeight = configManager.phone().screenHeight;
    _pixelsPerHidUnit = configManager.phone().hidSensitivity;
    _moveDelayMs = configManager.phone().moveDelayMs;

    Serial.printf("[BLE] 初始化: %s, 模式: %s\n",
                  _deviceName.c_str(), getModeName());

    if (_mode == HIDMode::KEYBOARD) {
        return initKeyboardMode(_deviceName.c_str());
    } else {
        return initMouseMode(_deviceName.c_str());
    }
}

void BLEHIDDevice::end() {
    if (_keyboard) {
        _keyboard->end();
        destroyKeyboardImpl(_keyboard);
        _keyboard = nullptr;
    }
    if (_mouse) {
        destroyMouseImpl(_mouse);
        _mouse = nullptr;
    }
    _initialized = false;
}

// ============================================
// 模式初始化
// ============================================
bool BLEHIDDevice::initKeyboardMode(const char* name) {
    _keyboard = createKeyboardImpl(name, "BlueHIDFlow");
    if (!_keyboard) {
        Serial.println("[BLE] 键盘模式内存分配失败");
        return false;
    }
    _keyboard->begin();
    _initialized = true;
    Serial.println("[BLE] 键盘模式启动完成，等待配对...");
    return true;
}

bool BLEHIDDevice::initMouseMode(const char* name) {
    _mouse = createMouseImpl(name, "BlueHIDFlow",
                             _screenWidth, _screenHeight,
                             _pixelsPerHidUnit, _moveDelayMs);
    if (!_mouse) {
        Serial.println("[BLE] 鼠标模式内存分配失败");
        return false;
    }
    _mouse->begin();
    _initialized = true;
    Serial.println("[BLE] 鼠标模式启动完成，等待配对...");
    return true;
}

// ============================================
// 连接状态
// ============================================
bool BLEHIDDevice::isConnected() {
    if (_mode == HIDMode::KEYBOARD && _keyboard) {
        return _keyboard->isConnected();
    } else if (_mode == HIDMode::MOUSE && _mouse) {
        return _mouse->isConnected();
    }
    return false;
}

bool BLEHIDDevice::isPaired() {
    if (_mode == HIDMode::KEYBOARD && _keyboard) {
        return _keyboard->isPaired();
    } else if (_mode == HIDMode::MOUSE && _mouse) {
        return _mouse->isPaired();
    }
    return false;
}

const char* BLEHIDDevice::getModeName() const {
    switch (_mode) {
        case HIDMode::KEYBOARD: return "键盘模式";
        case HIDMode::MOUSE:    return "鼠标模式";
        default:                return "未知";
    }
}

// ============================================
// 广播重启
// ============================================
void BLEHIDDevice::handleAdvertisingRestart() {
    // HijelHID 库内部处理断连重连
}

// ============================================
// 屏幕配置（鼠标模式）
// ============================================
void BLEHIDDevice::setScreenSize(uint16_t width, uint16_t height) {
    _screenWidth = width;
    _screenHeight = height;
    if (_mouse) {
        _mouse->setScreenSize(width, height);
    }
}

void BLEHIDDevice::setHIDSensitivity(float pixelsPerHidUnit) {
    _pixelsPerHidUnit = pixelsPerHidUnit;
    if (_mouse) {
        _mouse->setHIDSensitivity(pixelsPerHidUnit);
    }
}

void BLEHIDDevice::setMoveDelay(uint16_t delayMs) {
    _moveDelayMs = delayMs;
    if (_mouse) {
        _mouse->setMoveDelay(delayMs);
    }
}

// ============================================
// 键盘操作
// ============================================
bool BLEHIDDevice::typeString(const char* text) {
    if (_mode != HIDMode::KEYBOARD || !_keyboard) {
        Serial.println("[BLE] typeString 仅支持键盘模式");
        return false;
    }
    if (!_keyboard->isPaired()) {
        Serial.println("[BLE] 键盘未配对，无法输入");
        return false;
    }
    _keyboard->print(text);
    return true;
}

bool BLEHIDDevice::typeLine(const char* text) {
    if (_mode != HIDMode::KEYBOARD || !_keyboard) {
        Serial.println("[BLE] typeLine 仅支持键盘模式");
        return false;
    }
    if (!_keyboard->isPaired()) {
        Serial.println("[BLE] 键盘未配对，无法输入");
        return false;
    }
    _keyboard->println(text);
    return true;
}

bool BLEHIDDevice::keyPress(uint16_t keyCode) {
    if (_mode != HIDMode::KEYBOARD || !_keyboard) {
        Serial.println("[BLE] keyPress 仅支持键盘模式");
        return false;
    }
    if (!_keyboard->isPaired()) {
        Serial.println("[BLE] 键盘未配对，无法按键");
        return false;
    }

    // 原始 HID 扫描码（高位 0x0100 标记，低 8 位为 HID 码）
    if (keyCode & 0x0100) {
        uint8_t hidKey = keyCode & 0xFF;
        _keyboard->tapKey(hidKey);
        return true;
    }

    // 判断是否为媒体键/浏览器键
    uint16_t mediaKey = mapKeyCodeToMediaKey((KeyCode)keyCode);
    if (mediaKey != 0) {
        _keyboard->tapMediaKey(mediaKey);
        return true;
    }

    // 普通键
    uint8_t hidKey = mapKeyCodeToHID((KeyCode)keyCode);
    if (hidKey != 0) {
        _keyboard->tapKey(hidKey);
        return true;
    }

    Serial.printf("[BLE] 未知按键码: 0x%04X\n", keyCode);
    return false;
}

bool BLEHIDDevice::keyCombo(uint16_t keyCode, uint8_t modifiers) {
    if (_mode != HIDMode::KEYBOARD || !_keyboard) {
        Serial.println("[BLE] keyCombo 仅支持键盘模式");
        return false;
    }
    if (!_keyboard->isPaired()) {
        Serial.println("[BLE] 键盘未配对，无法发送组合键");
        return false;
    }

    // 原始 HID 扫描码（高位 0x0100 标记）
    if (keyCode & 0x0100) {
        uint8_t hidKey = keyCode & 0xFF;
        _keyboard->tapKeyWithModifier(hidKey, modifiers);
        return true;
    }

    // 媒体键不支持组合键修饰，直接发送
    uint16_t mediaKey = mapKeyCodeToMediaKey((KeyCode)keyCode);
    if (mediaKey != 0) {
        _keyboard->tapMediaKey(mediaKey);
        return true;
    }

    uint8_t hidKey = mapKeyCodeToHID((KeyCode)keyCode);
    if (hidKey == 0) {
        Serial.printf("[BLE] 未知按键码: 0x%04X\n", keyCode);
        return false;
    }

    _keyboard->tapKeyWithModifier(hidKey, modifiers);
    return true;
}

void BLEHIDDevice::keyRelease() {
    if (_keyboard) {
        _keyboard->releaseAll();
    }
}

// ============================================
// 鼠标操作
// ============================================
bool BLEHIDDevice::tapAbsolute(uint16_t screenX, uint16_t screenY, uint16_t duration_ms) {
    if (_mode != HIDMode::MOUSE || !_mouse) {
        Serial.println("[BLE] tapAbsolute 仅支持鼠标模式");
        return false;
    }
    if (!_mouse->isPaired()) {
        Serial.println("[BLE] 鼠标未配对，无法点击");
        return false;
    }

    Serial.printf("[BLE] 点击: (%d, %d), 时长: %dms\n", screenX, screenY, duration_ms);

    _mouse->moveToScreen(screenX, screenY);
    _mouse->clickLeft(duration_ms);

    return true;
}

bool BLEHIDDevice::swipeAbsolute(uint16_t fromX, uint16_t fromY,
                                  uint16_t toX, uint16_t toY,
                                  uint16_t duration_ms) {
    if (_mode != HIDMode::MOUSE || !_mouse) {
        Serial.println("[BLE] swipeAbsolute 仅支持鼠标模式");
        return false;
    }
    if (!_mouse->isPaired()) {
        Serial.println("[BLE] 鼠标未配对，无法滑动");
        return false;
    }

    Serial.printf("[BLE] 滑动: (%d,%d) -> (%d,%d), 时长: %dms\n",
                  fromX, fromY, toX, toY, duration_ms);

    _mouse->moveToScreen(fromX, fromY);

    _mouse->pressLeft();
    vTaskDelay(pdMS_TO_TICKS(50));

    int16_t dx = (int16_t)((toX - fromX) / _pixelsPerHidUnit);
    int16_t dy = (int16_t)((toY - fromY) / _pixelsPerHidUnit);

    _mouse->moveToHIDWithDuration(dx, dy, duration_ms);

    int16_t maxSteps = (abs(dx) + abs(dy)) / 127 + 2;
    uint32_t waitMs = maxSteps * 12;
    if (waitMs < duration_ms) waitMs = duration_ms;
    waitMs += 50;
    vTaskDelay(pdMS_TO_TICKS(waitMs));

    _mouse->releaseLeft();

    return true;
}

bool BLEHIDDevice::swipeDirection(SwipeDirection direction, uint16_t duration_ms) {
    if (_mode != HIDMode::MOUSE || !_mouse) {
        Serial.println("[BLE] swipeDirection 仅支持鼠标模式");
        return false;
    }

    uint16_t left   = _screenWidth / 10;
    uint16_t right  = _screenWidth * 9 / 10;
    uint16_t top    = _screenHeight / 5;
    uint16_t bottom = _screenHeight * 4 / 5;
    uint16_t centerX = _screenWidth / 2;
    uint16_t centerY = _screenHeight / 2;

    uint16_t fromX, fromY, toX, toY;

    switch (direction) {
        case SwipeDirection::UP:
            fromX = centerX; fromY = bottom;
            toX = centerX;   toY = top;
            break;
        case SwipeDirection::DOWN:
            fromX = centerX; fromY = top;
            toX = centerX;   toY = bottom;
            break;
        case SwipeDirection::LEFT:
            fromX = right;  fromY = centerY;
            toX = left;     toY = centerY;
            break;
        case SwipeDirection::RIGHT:
            fromX = left;   fromY = centerY;
            toX = right;    toY = centerY;
            break;
        default:
            return false;
    }

    return swipeAbsolute(fromX, fromY, toX, toY, duration_ms);
}

// ============================================
// 模式切换
// ============================================
bool BLEHIDDevice::switchMode(HIDMode newMode) {
    if (newMode == _mode) {
        Serial.printf("[BLE] 已在 %s 模式，无需切换\n", getModeName());
        return true;
    }

    Serial.printf("[BLE] 切换模式: %s -> %s，即将重启...\n",
                  getModeName(),
                  newMode == HIDMode::KEYBOARD ? "键盘模式" : "鼠标模式");

    configManager.bluetooth().hidMode = newMode;
    configManager.save();

    delay(300);
    ESP.restart();
    return true;
}

// ============================================
// 校准（鼠标模式）
// ============================================
bool BLEHIDDevice::calibrateMove(int hidUnitsX, int hidUnitsY,
                                  int delayMs, bool stepByStep) {
    if (_mode != HIDMode::MOUSE || !_mouse) {
        Serial.println("[BLE] calibrateMove 仅支持鼠标模式");
        return false;
    }
    if (!_mouse->isPaired()) {
        Serial.println("[BLE] 鼠标未配对，无法校准");
        return false;
    }

    Serial.printf("[BLE] 校准移动: X=%d, Y=%d, 延迟=%dms, 逐步=%d\n",
                  hidUnitsX, hidUnitsY, delayMs, stepByStep);

    if (stepByStep) {
        int stepsX = abs(hidUnitsX);
        int stepsY = abs(hidUnitsY);
        int signX = hidUnitsX >= 0 ? 1 : -1;
        int signY = hidUnitsY >= 0 ? 1 : -1;
        int maxSteps = stepsX > stepsY ? stepsX : stepsY;

        for (int i = 0; i < maxSteps; i++) {
            int8_t dx = (i < stepsX) ? (int8_t)(1 * signX) : 0;
            int8_t dy = (i < stepsY) ? (int8_t)(1 * signY) : 0;
            _mouse->moveHID(dx, dy);
            Serial.printf("[BLE] 步骤 %d/%d: dx=%d, dy=%d\n", i + 1, maxSteps, dx, dy);
            delay(delayMs);
        }
    } else {
        _mouse->moveToHID((int16_t)hidUnitsX, (int16_t)hidUnitsY);

        int16_t maxSteps = (abs(hidUnitsX) + abs(hidUnitsY)) / 127 + 2;
        uint32_t waitMs = maxSteps * 12;
        if (waitMs < 100) waitMs = 100;
        vTaskDelay(pdMS_TO_TICKS(waitMs));
    }

    return true;
}