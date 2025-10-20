#include <TTL.h>

void initTTL() {
    Serial.println("Initializing laser TTL...");
    pinMode(LASER_ON, OUTPUT);
    digitalWriteFast(LASER_ON, LOW); // Start with laser on
    delay(100); // Create blinking effect
    digitalWriteFast(LASER_ON, HIGH); // Laser off
    delay(100);
    digitalWriteFast(LASER_ON, LOW); // Laser on
    delay(100);
    Serial.println("Laser initialized");
}

void LASER_ON_FAST() {
    digitalWriteFast(LASER_ON, LOW);
}

void LASER_OFF_FAST() {
    digitalWriteFast(LASER_ON, HIGH);
}