#include "Laser.hpp"

Laser::Laser(int _laserPin) : laserPin(_laserPin) {}

void Laser::init() {
    Serial.println("Initializing laser TTL...");
    pinMode(laserPin, OUTPUT);
    on();
    delay(100); // blinking...
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

void Laser::deinit() {
    off();
    pinMode(laserPin, INPUT);
    Serial.println("Laser deinitialised");
}