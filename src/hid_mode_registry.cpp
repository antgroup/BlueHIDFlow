// BlueHIDFlow HID 模式注册表实现
#include "ihid_mode.h"

HIDModeRegistry& HIDModeRegistry::instance() {
    static HIDModeRegistry registry;
    return registry;
}

void HIDModeRegistry::registerMode(IHIDMode* mode) {
    if (_count >= MAX_MODES) {
        Serial.printf("[HID] 模式注册表已满，无法注册: %s\n", mode->name());
        return;
    }
    _modes[_count++] = mode;
    Serial.printf("[HID] 注册 HID 模式: %s (id=%d)\n", mode->name(), (int)mode->id());
}

IHIDMode* HIDModeRegistry::getMode(HIDMode id) {
    for (int i = 0; i < _count; i++) {
        if (_modes[i]->id() == id) {
            return _modes[i];
        }
    }
    return nullptr;
}

IHIDMode* HIDModeRegistry::findByName(const char* name) {
    for (int i = 0; i < _count; i++) {
        if (strcmp(_modes[i]->name(), name) == 0) {
            return _modes[i];
        }
    }
    return nullptr;
}