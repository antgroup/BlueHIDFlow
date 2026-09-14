// BlueHIDFlow BLE HID - 鼠标模式实现
// 独立编译单元，避免与 Keyboard 库的枚举冲突
//
// MouseImpl 继承 IMouseImpl 接口，封装 HijelBLEMouse

#include "ble_hid.h"
#include "config_manager.h"
#include <HijelHID_BLEMouse.h>

// ============================================
// 鼠标实现类
// ============================================
struct BLEHIDDevice::MouseImpl : public IMouseImpl {
    HijelBLEMouse bleMouse;
    int32_t currentX;  // 当前鼠标位置（HID 坐标系）
    int32_t currentY;
    uint16_t screenWidth;
    uint16_t screenHeight;
    float pixelsPerHidUnit;
    uint16_t moveDelayMs;

    MouseImpl(const char* name, const char* manufacturer,
              uint16_t sw, uint16_t sh, float sens, uint16_t delay)
        : bleMouse(name, manufacturer, 100, 3, false),
          currentX(0), currentY(0),
          screenWidth(sw), screenHeight(sh),
          pixelsPerHidUnit(sens), moveDelayMs(delay) {}

    void begin() override {
        bleMouse.setLogLevel(HIDLogLevel::Normal);
        bleMouse.begin();
    }

    // 接口方法
    bool isConnected() override { return bleMouse.isConnected(); }
    bool isPaired() override { return bleMouse.isPaired(); }
    void clickLeft(uint16_t duration_ms) override { bleMouse.click(MouseButton::Left, duration_ms); }
    void pressLeft() override { bleMouse.press(MouseButton::Left); }
    void releaseLeft() override { bleMouse.release(MouseButton::Left); }
    void moveHID(int8_t dx, int8_t dy) override { bleMouse.move(dx, dy); }
    void moveToHID(int16_t dx, int16_t dy) override { bleMouse.moveTo(dx, dy); }
    void moveToHIDWithDuration(int16_t dx, int16_t dy, uint16_t duration_ms) override {
        bleMouse.moveTo(dx, dy, duration_ms);
    }

    void setScreenSize(uint16_t width, uint16_t height) override {
        screenWidth = width;
        screenHeight = height;
    }

    void setHIDSensitivity(float pixelsPerHidUnit) override {
        this->pixelsPerHidUnit = pixelsPerHidUnit;
    }

    void setMoveDelay(uint16_t delayMs) override {
        moveDelayMs = delayMs;
    }

    void moveToScreen(uint16_t screenX, uint16_t screenY) override {
        // 归零：快速移动到屏幕左上角
        for (int i = 0; i < 40; i++) {
            bleMouse.move(-127, -127);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        currentX = 0;
        currentY = 0;

        int16_t hidX = (int16_t)(screenX / pixelsPerHidUnit);
        int16_t hidY = (int16_t)(screenY / pixelsPerHidUnit);

        // 使用库的 moveTo + durationMs 控制移动速度，避免 Android 鼠标加速
        // 持续时间按距离计算：2ms/HID单位，最少 200ms
        uint32_t duration = max(abs(hidX), abs(hidY)) * 2;
        if (duration < 200) duration = 200;
        bleMouse.moveTo(hidX, hidY, duration);

        // 等待移动完成（库内部异步发送）
        int16_t maxSteps = (abs(hidX) + abs(hidY)) / 127 + 2;
        uint32_t waitMs = maxSteps * 12;
        if (waitMs < duration) waitMs = duration;
        waitMs += 50;
        vTaskDelay(pdMS_TO_TICKS(waitMs));

        currentX = hidX;
        currentY = hidY;
    }
};

// ============================================
// 工厂/销毁函数
// ============================================
IMouseImpl* BLEHIDDevice::createMouseImpl(const char* name, const char* manufacturer,
                                           uint16_t sw, uint16_t sh, float sens, uint16_t delay) {
    return new MouseImpl(name, manufacturer, sw, sh, sens, delay);
}

void BLEHIDDevice::destroyMouseImpl(IMouseImpl* p) {
    delete p;
}