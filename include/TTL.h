#include <Arduino.h>

#define LASER_ON 7

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


// Fast laser control macros
#define LASER_ON_FAST() GPIO2_DR_CLEAR = (1 << 17)
#define LASER_OFF_FAST() GPIO2_DR_SET = (1 << 17)