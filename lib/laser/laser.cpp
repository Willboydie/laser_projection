#include "laser/Laser.hpp"

Laser::Laser(int _laserPin) : laserPin(_laserPin) {}

void Laser::init() {
    Serial.println("Initializing laser TTL...");
    pinMode(laserPin, OUTPUT);
    on();
    delay(100); // Create blinking effect
    off();
    delay(100);
    on();
    delay(100);
    off();
    Serial.println("Laser initialized");
}

void Laser::update(uint8_t* _laserOn) {
    if (laserOn != *_laserOn) {
        if (laserOn) {
            off();
        } else {
            on();
        }
    }
}

void Laser::on() {
    digitalWriteFast(laserPin, LOW);
    laserOn = true;
}

void Laser::off() {
    digitalWriteFast(laserPin, HIGH);
    laserOn = false;
}