#pragma once

#include <Arduino.h>

class Laser {
public:
    Laser(int _laserPin);
    ~Laser() = default;
    void init();
    void update(uint8_t laserState);

private:
    int laserPin;
    bool laserOn = false;
};