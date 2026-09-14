// BlueHIDFlow BLE HID - 键盘模式实现
// 独立编译单元，避免与 Mouse 库的枚举冲突
//
// KeyboardImpl 继承 IKeyboardImpl 接口，封装 HijelHID_BLEKeyboard

#include "ble_hid.h"
#include "config_manager.h"
#include <HijelHID_BLEKeyboard.h>

// ============================================
// 键盘实现类
// ============================================
struct BLEHIDDevice::KeyboardImpl : public IKeyboardImpl {
    HijelHID_BLEKeyboard bleKeyboard;

    KeyboardImpl(const char* name, const char* manufacturer)
        : bleKeyboard(name, manufacturer, 100) {}

    void begin() override {
        bleKeyboard.setTapDelay(40);   // 增加按键保持时间，提高 Android 兼容性
        bleKeyboard.setKeyGap(40);     // 增加按键间隔
        bleKeyboard.setLogLevel(HIDLogLevel::Normal);
        bleKeyboard.begin();
    }

    void end() override {
        bleKeyboard.end();
    }

    bool isConnected() const override { return bleKeyboard.isConnected(); }
    bool isPaired() const override { return bleKeyboard.isPaired(); }
    void print(const char* text) override { bleKeyboard.print(text); }
    void println(const char* text) override { bleKeyboard.println(text); }
    void tapKey(uint8_t key) override { bleKeyboard.tap(key); }
    void tapKeyWithModifier(uint8_t key, uint8_t modifiers) override { bleKeyboard.tap(key, modifiers); }
    void tapMediaKey(uint16_t key) override { bleKeyboard.tap(key); }
    void releaseAll() override { bleKeyboard.releaseAll(); }
};

// ============================================
// 工厂/销毁函数
// ============================================
IKeyboardImpl* BLEHIDDevice::createKeyboardImpl(const char* name, const char* manufacturer) {
    return new KeyboardImpl(name, manufacturer);
}

void BLEHIDDevice::destroyKeyboardImpl(IKeyboardImpl* p) {
    delete p;
}

// ============================================
// 按键码映射 - 普通键
// ============================================
uint8_t BLEHIDDevice::mapKeyCodeToHID(KeyCode code) {
    switch (code) {
        case KeyCode::HOME:      return KEY_NONE;  // HOME 通过媒体键 AC Home 发送
        case KeyCode::BACK:      return KEY_NONE;  // BACK 通过媒体键 AC Back 发送
        case KeyCode::ENTER:     return KEY_RETURN;
        case KeyCode::ESCAPE:    return KEY_ESCAPE;
        case KeyCode::BACKSPACE: return KEY_BACKSPACE;
        case KeyCode::DELETE_KEY:return KEY_DELETE;
        case KeyCode::TAB:       return KEY_TAB;
        case KeyCode::SPACE:     return KEY_SPACE;
        case KeyCode::UP:        return KEY_UP;
        case KeyCode::DOWN:      return KEY_DOWN;
        case KeyCode::LEFT:      return KEY_LEFT;
        case KeyCode::RIGHT:     return KEY_RIGHT;
        case KeyCode::PAGE_UP:   return KEY_PAGE_UP;
        case KeyCode::PAGE_DOWN: return KEY_PAGE_DOWN;
        case KeyCode::F1:        return KEY_F1;
        case KeyCode::F2:        return KEY_F2;
        case KeyCode::F3:        return KEY_F3;
        case KeyCode::F4:        return KEY_F4;
        case KeyCode::F5:        return KEY_F5;
        case KeyCode::F6:        return KEY_F6;
        case KeyCode::F7:        return KEY_F7;
        case KeyCode::F8:        return KEY_F8;
        case KeyCode::F9:        return KEY_F9;
        case KeyCode::F10:       return KEY_F10;
        case KeyCode::F11:       return KEY_F11;
        case KeyCode::F12:       return KEY_F12;
        default:                 return KEY_NONE;
    }
}

// ============================================
// 按键码映射 - 媒体键/浏览器键
// ============================================
uint16_t BLEHIDDevice::mapKeyCodeToMediaKey(KeyCode code) {
    switch (code) {
        case KeyCode::VOLUME_UP:    return MEDIA_VOLUME_UP;
        case KeyCode::VOLUME_DOWN:  return MEDIA_VOLUME_DOWN;
        case KeyCode::MUTE:         return MEDIA_MUTE;
        case KeyCode::PLAY_PAUSE:   return MEDIA_PLAY_PAUSE;
        case KeyCode::NEXT_TRACK:   return MEDIA_NEXT_TRACK;
        case KeyCode::PREV_TRACK:   return MEDIA_PREV_TRACK;
        case KeyCode::HOME:         return MEDIA_BROWSER_HOME;   // AC Home → Android 回桌面
        case KeyCode::BACK:         return MEDIA_BROWSER_BACK;   // AC Back → Android 返回
        case KeyCode::BROWSER_BACK: return MEDIA_BROWSER_BACK;
        case KeyCode::BROWSER_HOME: return MEDIA_BROWSER_HOME;
        case KeyCode::DPAD_CENTER:  return 0x0214;  // Consumer AC Select
        default:                    return 0;
    }
}