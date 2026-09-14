// BlueHIDFlow BLE HID - 统一键盘/鼠标模式
// v0.8.0 - 2026-04-21 重构版本
//
// 基于 HijelHID 系列库实现 BLE HID：
// - 键盘模式（主要）：文字输入、系统键、媒体键、快捷键
// - 鼠标模式（次要）：精确定位点击、滑动操作
// - 模式切换通过重启实现（两个库不能同时运行）
//
#ifndef BLE_HID_H
#define BLE_HID_H

#include <Arduino.h>
#include "config_manager.h"

// HIDMode 已在 config_manager.h 中定义

// ============================================
// 滑动方向枚举
// ============================================
enum class SwipeDirection : uint8_t {
    UP = 0,
    DOWN = 1,
    LEFT = 2,
    RIGHT = 3
};

// ============================================
// 按键码 - 用于 MQTT 命令映射
// ============================================
enum class KeyCode : uint16_t {
    // 导航
    HOME = 0x01,
    BACK = 0x02,        // 映射到 KEY_ESCAPE
    ENTER = 0x03,
    ESCAPE = 0x04,
    BACKSPACE = 0x05,
    DELETE_KEY = 0x06,
    TAB = 0x07,
    SPACE = 0x08,
    // 方向键
    UP = 0x10,
    DOWN = 0x11,
    LEFT = 0x12,
    RIGHT = 0x13,
    // 翻页
    PAGE_UP = 0x14,
    PAGE_DOWN = 0x15,
    // 功能键
    F1 = 0x20, F2, F3, F4, F5, F6,
    F7, F8, F9, F10, F11, F12,
    // 媒体键
    VOLUME_UP = 0x30,
    VOLUME_DOWN = 0x31,
    MUTE = 0x32,
    PLAY_PAUSE = 0x33,
    NEXT_TRACK = 0x34,
    PREV_TRACK = 0x35,
    // 浏览器键
    BROWSER_BACK = 0x40,
    BROWSER_HOME = 0x41,
    // 焦点激活（Consumer AC Select 0x0214，Android 可能映射为 DPAD_CENTER）
    DPAD_CENTER = 0x42,
};

// ============================================
// 修饰键标志
// ============================================
enum class KeyModifier : uint8_t {
    NONE  = 0x00,
    CTRL  = 0x01,
    SHIFT = 0x02,
    ALT   = 0x04,
    GUI   = 0x08,      // Win/Cmd/Meta
};

// ============================================
// 键盘实现接口（抽象基类）
// 具体实现在 ble_hid_keyboard.cpp 中，隐藏 HijelHID 库细节
// ============================================
class IKeyboardImpl {
public:
    virtual ~IKeyboardImpl() = default;
    virtual void begin() = 0;
    virtual void end() = 0;
    virtual bool isConnected() const = 0;
    virtual bool isPaired() const = 0;
    virtual void print(const char* text) = 0;
    virtual void println(const char* text) = 0;
    virtual void tapKey(uint8_t key) = 0;
    virtual void tapKeyWithModifier(uint8_t key, uint8_t modifiers) = 0;
    virtual void tapMediaKey(uint16_t key) = 0;
    virtual void releaseAll() = 0;
};

// ============================================
// 鼠标实现接口（抽象基类）
// 具体实现在 ble_hid_mouse.cpp 中，隐藏 HijelHID 库细节
// ============================================
class IMouseImpl {
public:
    virtual ~IMouseImpl() = default;
    virtual void begin() = 0;
    virtual bool isConnected() = 0;
    virtual bool isPaired() = 0;
    virtual void clickLeft(uint16_t duration_ms) = 0;
    virtual void pressLeft() = 0;
    virtual void releaseLeft() = 0;
    virtual void moveHID(int8_t dx, int8_t dy) = 0;
    virtual void moveToHID(int16_t dx, int16_t dy) = 0;
    virtual void moveToHIDWithDuration(int16_t dx, int16_t dy, uint16_t duration_ms) = 0;
    virtual void moveToScreen(uint16_t screenX, uint16_t screenY) = 0;
    virtual void setScreenSize(uint16_t width, uint16_t height) = 0;
    virtual void setHIDSensitivity(float pixelsPerHidUnit) = 0;
    virtual void setMoveDelay(uint16_t delayMs) = 0;
};

// ============================================
// BLE HID 设备 - 统一接口
// ============================================
class BLEHIDDevice {
public:
    BLEHIDDevice();
    ~BLEHIDDevice();

    // ---- 生命周期 ----
    bool begin(const char* deviceName = nullptr, HIDMode mode = HIDMode::KEYBOARD);
    void end();

    // ---- 连接状态 ----
    bool isConnected();
    bool isPaired();
    HIDMode getCurrentMode() const { return _mode; }
    const char* getDeviceName() const { return _deviceName.c_str(); }
    const char* getModeName() const;

    // ---- 广播重启（在 loop() 中调用）----
    void handleAdvertisingRestart();

    // ---- 屏幕配置（鼠标模式）----
    void setScreenSize(uint16_t width, uint16_t height);
    uint16_t getScreenWidth() const { return _screenWidth; }
    uint16_t getScreenHeight() const { return _screenHeight; }
    void setHIDSensitivity(float pixelsPerHidUnit);
    float getHIDSensitivity() const { return _pixelsPerHidUnit; }
    void setMoveDelay(uint16_t delayMs);
    uint16_t getMoveDelay() const { return _moveDelayMs; }

    // ---- 键盘操作（键盘模式）----
    bool typeString(const char* text);
    bool typeLine(const char* text);         // 文字 + 回车
    bool keyPress(uint16_t keyCode);          // 单键点击
    bool keyCombo(uint16_t keyCode, uint8_t modifiers);  // 组合键
    void keyRelease();

    // ---- 鼠标操作（鼠标模式）----
    bool tapAbsolute(uint16_t screenX, uint16_t screenY, uint16_t duration_ms = 50);
    bool swipeAbsolute(uint16_t fromX, uint16_t fromY,
                       uint16_t toX, uint16_t toY,
                       uint16_t duration_ms = 300);
    bool swipeDirection(SwipeDirection direction, uint16_t duration_ms = 300);

    // ---- 模式切换（保存到 NVS 并重启）----
    bool switchMode(HIDMode newMode);

    // ---- 校准（鼠标模式）----
    bool calibrateMove(int hidUnitsX, int hidUnitsY,
                       int delayMs = 50, bool stepByStep = false);

    // ---- 公有状态（供回调使用）----
    bool _connected = false;
    bool _needRestartAdvertising = false;

private:
    bool _initialized = false;
    String _deviceName;
    HIDMode _mode = HIDMode::KEYBOARD;

    // 屏幕配置（鼠标模式）
    uint16_t _screenWidth = 1080;
    uint16_t _screenHeight = 2400;
    float _pixelsPerHidUnit = 2.5f;
    uint16_t _moveDelayMs = 100;

    // Pimpl 模式：通过接口指针隐藏 HijelHID 库实现细节
    // 具体实现类（KeyboardImpl/MouseImpl）定义在独立编译单元中
    struct KeyboardImpl;
    struct MouseImpl;
    IKeyboardImpl* _keyboard = nullptr;
    IMouseImpl* _mouse = nullptr;

    // Pimpl 工厂/销毁（定义在独立编译单元中，避免枚举冲突）
    static IKeyboardImpl* createKeyboardImpl(const char* name, const char* manufacturer);
    static void destroyKeyboardImpl(IKeyboardImpl* p);
    static IMouseImpl* createMouseImpl(const char* name, const char* manufacturer,
                                       uint16_t sw, uint16_t sh, float sens, uint16_t delay);
    static void destroyMouseImpl(IMouseImpl* p);

    // 内部方法
    bool initKeyboardMode(const char* name);
    bool initMouseMode(const char* name);
    uint8_t mapKeyCodeToHID(KeyCode code);
    uint16_t mapKeyCodeToMediaKey(KeyCode code);
};

// 全局实例
extern BLEHIDDevice bleHid;

#endif // BLE_HID_H