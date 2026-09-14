#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    Serial.println("Test compile successful!");
}

void loop() {
    delay(1000);
    Serial.println("Running...");
}
