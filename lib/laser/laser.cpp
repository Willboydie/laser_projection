#include "Laser.hpp"

Laser::Laser(int _laserPin, float initialDutyCycle) 
: laserPin(_laserPin)
, dutyCycle(initialDutyCycle) {
    setDutyCycle(initialDutyCycle);
}

void Laser::init() {
    Serial.println("Initializing laser TTL...");
    pinMode(laserPin, OUTPUT);
    analogWriteResolution(12);
    analogWriteFrequency(laserPin, DUTY_CYCLE_FREQUENCY);
    laserOn = false;
    Serial.println("Laser initialized successfully");
}

void Laser::setDutyCycle(float _dutyCycle) {
    if (_dutyCycle <= 0.0f) {
        analogWrite(laserPin, 0);
        return;
    }
    if (_dutyCycle >= 1.0f) {
        analogWrite(laserPin, 4095);
        return;
    }
    dutyCycle = _dutyCycle;
    uint16_t value = (uint16_t)(dutyCycle * 4095.0f);
    analogWrite(laserPin, value);
}

void Laser::update(bool laserState) {
    if (laserState) {
        if (!laserOn) { 
            laserOn = true;
        }
        analogWrite(laserPin, (uint16_t)(dutyCycle * 4095.0f));
    } else {
        if (laserOn) off();
    }
}

void Laser::off() {
    analogWrite(laserPin, 0);
    laserOn = false;
}

void Laser::deinit() {
    off();
    pinMode(laserPin, INPUT);
    Serial.println("Laser deinitialised");
}