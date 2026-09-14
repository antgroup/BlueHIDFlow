// BlueHIDFlow HID 模式接口
// 允许第三方扩展新的 HID 模式
#ifndef IHID_MODE_H
#define IHID_MODE_H

#include <Arduino.h>
#include "config_manager.h"
#include "ble_hid.h"

// ============================================================
// HID 模式接口
// 每种 HID 模式（键盘、鼠标等）实现此接口
// ============================================================
class IHIDMode {
public:
    virtual ~IHIDMode() = default;

    // 模式标识
    virtual const char* name() const = 0;   // "keyboard", "mouse"
    virtual HIDMode id() const = 0;         // HIDMode 枚举值

    // 生命周期
    virtual bool begin(const char* deviceName) = 0;
    virtual void end() = 0;

    // 连接状态
    virtual bool isConnected() const = 0;
    virtual bool isPaired() const = 0;

    // 键盘操作（键盘模式实现，鼠标模式返回 false）
    virtual bool typeString(const char* text) = 0;
    virtual bool keyPress(uint16_t keyCode) = 0;
    virtual bool keyCombo(uint16_t keyCode, uint8_t modifiers) = 0;
    virtual void keyRelease() = 0;

    // 鼠标操作（鼠标模式实现，键盘模式返回 false）
    virtual bool tapAbsolute(uint16_t screenX, uint16_t screenY, uint16_t duration_ms = 50) = 0;
    virtual bool swipeAbsolute(uint16_t fromX, uint16_t fromY,
                               uint16_t toX, uint16_t toY,
                               uint16_t duration_ms = 300) = 0;
    virtual bool swipeDirection(SwipeDirection direction, uint16_t duration_ms = 300) = 0;
    virtual bool calibrateMove(int hidUnitsX, int hidUnitsY,
                               int delayMs = 50, bool stepByStep = false) = 0;

    // 屏幕配置（鼠标模式使用）
    virtual void setScreenSize(uint16_t width, uint16_t height) = 0;
    virtual void setHIDSensitivity(float pixelsPerHidUnit) = 0;
    virtual void setMoveDelay(uint16_t delayMs) = 0;
    virtual uint16_t getScreenWidth() const = 0;
    virtual uint16_t getScreenHeight() const = 0;
    virtual float getHIDSensitivity() const = 0;
    virtual uint16_t getMoveDelay() const = 0;
};

// ============================================================
// HID 模式注册表
// ============================================================
class HIDModeRegistry {
public:
    static HIDModeRegistry& instance();

    void registerMode(IHIDMode* mode);
    IHIDMode* getMode(HIDMode id);
    IHIDMode* findByName(const char* name);
    int count() const { return _count; }

private:
    HIDModeRegistry() = default;
    static constexpr int MAX_MODES = 4;
    IHIDMode* _modes[MAX_MODES];
    int _count = 0;
};

// ============================================================
// 自注册宏
// 用法: REGISTER_HID_MODE(MyHIDModeClass)
// ============================================================
#define REGISTER_HID_MODE(cls) \
    static cls _hid_mode_instance_##cls; \
    static bool _hid_mode_registered_##cls = [](){ \
        HIDModeRegistry::instance().registerMode(&_hid_mode_instance_##cls); \
        return true; }();

#endif // IHID_MODE_H