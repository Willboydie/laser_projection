#include <Arduino.h>
#include "../utils/constants.h"

class Debug {
public:
    static void msg(const char* text) {
        if (mode) {
            Serial.println(text);
        }
    }
    static bool mode = DEFAULT_DEBUG_SETTING;
};