#include <TTL.h>

void initTTL() {
    Serial.println("Initializing laser TTL...");
    pinMode(LASER_ON, OUTPUT);
    digitalWriteFast(LASER_ON, LOW); // Start with laser off
    delay(1000); // Create blinking effect
    digitalWriteFast(LASER_ON, HIGH); // Laser off
    delay(1000);
    digitalWriteFast(LASER_ON, LOW); // Laser on
    delay(1000);
    Serial.println("Laser initialized");
}
