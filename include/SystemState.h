#ifndef SYSTEMSTATE_H
#define SYSTEMSTATE_H

#include <Arduino.h>

class SystemState {
public:
    enum Mode { 
        MODE_WRITE, 
        MODE_PROJECT 
    };
    
private:
    Mode currentMode = MODE_WRITE;
    bool isProjecting = false;
    uint32_t currentDelayUs = 0;
    
public:
    // Mode management
    void setMode(Mode mode) { 
        currentMode = mode; 
        Serial.print("MODE: ");
        Serial.println(mode == MODE_WRITE ? "WRITE" : "PROJECT");
    }
    
    Mode getMode() const { return currentMode; }


    
    // Projection state
    void setProjecting(bool projecting) { isProjecting = projecting; }
    
    bool isCurrentlyProjecting() const { return isProjecting; }
    
    // Timing
    void setDelay(uint32_t delayUs) { currentDelayUs = delayUs; }
    uint32_t getDelay() const { return currentDelayUs; }
    
    // Status
    void printStatus() const {
        Serial.print("System State - Mode: ");
        Serial.print(currentMode == MODE_WRITE ? "WRITE" : "PROJECT");
        Serial.print(", Projecting: ");
        Serial.print(isProjecting ? "YES" : "NO");
        Serial.print(", Delay: ");
        Serial.print(currentDelayUs);
        Serial.println("μs");
    }
};

#endif
