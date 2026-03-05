#pragma once

#include <Arduino.h>
#include "../utils/quick_defs.h"
#include "../utils/constants.h"

class Laser {
public:
    Laser(int _laserPin, float initialDutyCycle);
    ~Laser() = default;
    void init();
    void deinit();

    void update(bool laserState);
    void off();

    void setDutyCycle(float _dutyCycle);
    float getDutyCycle() const { return dutyCycle; }

    bool isOn() const { return laserOn; }

private:
    int laserPin;
    bool laserOn = false;
    float dutyCycle;
};