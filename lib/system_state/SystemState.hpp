#pragma once

#include <Arduino.h>

enum Mode { 
    MODE_WRITE, 
    MODE_READY,
    MODE_PROJECT 
};

struct SystemState {
    Mode mode = MODE_READY;
    bool enabled = false;
    bool projecting = false;
};