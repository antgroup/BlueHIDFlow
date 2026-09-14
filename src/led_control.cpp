#include "led_control.h"

LEDController::LEDController()
    : _currentMode(LEDMode::OFF)
    , _targetMode(LEDMode::OFF)
    , _brightness(255)
{
}

void LEDController::begin() {
    pinMode(LED_R_PIN, OUTPUT);
    pinMode(LED_G_PIN, OUTPUT);
    pinMode(LED_B_PIN, OUTPUT);

    digitalWrite(LED_R_PIN, HIGH);
    digitalWrite(LED_G_PIN, HIGH);
    digitalWrite(LED_B_PIN, HIGH);

    _currentMode = LEDMode::OFF;
    _targetMode = LEDMode::OFF;

    Serial.println("[LED] Built-in RGB LED initialized");
}

void LEDController::setMode(LEDMode mode) {
    _targetMode = mode;
}

void LEDController::setBrightness(uint8_t brightness) {
    _brightness = brightness;
    applyColor();
}

void LEDController::onMQTTDisconnected() {
    Serial.println("[LED] MQTT disconnected -> RED");
    _targetMode = LEDMode::RED;
}

void LEDController::onMQTTConnected() {
    Serial.println("[LED] MQTT connected");
    _targetMode = LEDMode::OFF;
}

void LEDController::update() {
    if (_currentMode != _targetMode) {
        _currentMode = _targetMode;
        applyColor();
    }
}

void LEDController::applyColor() {
    switch (_currentMode) {
        case LEDMode::OFF:
            setColor(false, false, false);
            break;
        case LEDMode::RED:
            setColor(true, false, false);
            break;
        case LEDMode::BLUE:
            setColor(false, false, true);
            break;
        case LEDMode::GREEN:
            setColor(false, true, false);
            break;
        case LEDMode::WHITE:
            setColor(true, true, true);
            break;
        case LEDMode::YELLOW:
            setColor(true, true, false);
            break;
        case LEDMode::MAGENTA:
            setColor(true, false, true);
            break;
        case LEDMode::CYAN:
            setColor(false, true, true);
            break;
    }
}

void LEDController::setColor(bool r, bool g, bool b) {
    digitalWrite(LED_R_PIN, r ? LOW : HIGH);
    digitalWrite(LED_G_PIN, g ? LOW : HIGH);
    digitalWrite(LED_B_PIN, b ? LOW : HIGH);
}

void LEDController::loop() {
    update();
}

LEDController g_ledController;