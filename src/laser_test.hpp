#include <Arduino.h>


void laser_test() {
    Serial.println("Laser test started");
    

    digitalWriteFast(LASER_PIN, HIGH);
    delay(1000);
    digitalWriteFast(LASER_PIN, LOW);
    delay(1000);
    digitalWriteFast(LASER_PIN, HIGH);
    delay(1000);
    digitalWriteFast(LASER_PIN, LOW);
    delay(1000);
    digitalWriteFast(LASER_PIN, HIGH);
    delay(1000);
    digitalWriteFast(LASER_PIN, LOW);
    Serial.println("Laser test ended");
    while(true) { delay(1000); }
}