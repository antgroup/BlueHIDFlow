#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <Arduino.h>
#include "gpio_config.h"

enum class LEDMode {
    OFF,
    RED,
    BLUE,
    GREEN,
    WHITE,
    YELLOW,
    MAGENTA,
    CYAN
};

class LEDController {
public:
    LEDController();

    void begin();
    void loop();

    void setMode(LEDMode mode);
    LEDMode getMode() const { return _currentMode; }

    void setBrightness(uint8_t brightness);
    uint8_t getBrightness() const { return _brightness; }

    void onMQTTDisconnected();
    void onMQTTConnected();

    void update();

private:
    LEDMode _currentMode;
    LEDMode _targetMode;
    uint8_t _brightness;

    void applyColor();
    void setColor(bool r, bool g, bool b);
};

extern LEDController g_ledController;

#endif