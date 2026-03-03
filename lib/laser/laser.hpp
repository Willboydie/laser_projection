#pragma once

#include <Arduino.h>

class Laser {
public:
    Laser(int _laserPin);
    ~Laser() = default;
    void init();
    void update(uint8_t* _laserOn);

    void on();
    void off();
    bool isOn() const { return laserOn; }

private:
    int laserPin;
    bool laserOn = false;
};