#include <Arduino.h>

#define LASER_ON 40

void initTTL() {
    Serial.println("Initializing laser TTL...");
    pinMode(LASER_ON, OUTPUT);
    digitalWriteFast(LASER_ON, HIGH); // Start with laser off
    delay(1000); // Allow time for laser to power up
    digitalWriteFast(LASER_ON, LOW); // Laser off
    delay(1000);
    digitalWriteFast(LASER_ON, HIGH); // Laser on
    delay(1000);
    Serial.println("Laser initialized");
}

void toggleLaserOff() {
    digitalWriteFast(LASER_ON, LOW);
}

void toggleLaserOn() {
    digitalWriteFast(LASER_ON, HIGH);
}