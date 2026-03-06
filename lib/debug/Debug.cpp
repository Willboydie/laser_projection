#include "Debug.hpp"

void Debug::msg(const char* text) {
    if (mode) {
        Serial.println(text);
    }
}

bool Debug::mode = DEFAULT_DEBUG_SETTING;