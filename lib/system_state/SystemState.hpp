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

enum ProjectMode {
    PROJECT_MODE_POINT,
    PROJECT_MODE_PATH
};

struct SystemState {
    Mode mode = MODE_READY;
    PowerState powerState = POWER_AWAKE;
    ProjectMode projectMode = PROJECT_MODE_PATH;
    bool enabled = true;
    bool projecting = false;
};
