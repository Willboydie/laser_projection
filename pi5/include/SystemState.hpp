#pragma once

#include <iostream>


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
            std::cerr << "MODE: " << (mode == MODE_WRITE ? "WRITE" : "PROJECT") << std::endl;
        }
        
        Mode getMode() const { return currentMode; }
    
        
        // Projection state
        void setProjecting(bool projecting) { 
            isProjecting = projecting; 
        }
        
        bool isCurrentlyProjecting() const { return isProjecting; }
        
        // Timing
        void setDelay(uint32_t delayUs) { currentDelayUs = delayUs; }
        uint32_t getDelay() const { return currentDelayUs; }
        
    };