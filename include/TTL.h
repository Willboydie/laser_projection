#include <Arduino.h>

#define LASER_ON 7

void initTTL() {
    pinMode(LASER_ON, OUTPUT);
    digitalWrite(LASER_ON, HIGH);
}

void toggleLaserOff() {
    digitalWriteFast(LASER_ON, LOW);
}

void toggleLaserOn() {
    digitalWriteFast(LASER_ON, HIGH);
}