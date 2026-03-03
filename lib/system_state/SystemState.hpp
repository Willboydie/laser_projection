#pragma once

#include <Arduino.h>

enum Mode { 
    MODE_WRITE, 
    MODE_READY,
    MODE_PROJECT 
};

enum PowerState {
    POWER_AWAKE,
    POWER_SLEEPING
};

struct SystemState {
    Mode mode = MODE_READY;
    PowerState powerState = POWER_AWAKE;
    bool enabled = false;
    bool projecting = false;
};
