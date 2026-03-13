#pragma once

#include <Arduino.h>
#include "../utils/constants.h"

constexpr uint8_t DEBUG_HEADER = 0b11111111;

class Debug {
public:
    template<typename T>
    static void msg(const T& text) {
        if (mode) {
            Serial.write(DEBUG_HEADER);
            Serial.println(text);
        }
    }
    static bool mode;
};