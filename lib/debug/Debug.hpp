#pragma once

#include <Arduino.h>
#include "../utils/constants.h"

class Debug {
public:
    template<typename T>
    static void msg(const T& text) {
        if (mode) {
            Serial.println(text);
        }
    }
    static bool mode;
};