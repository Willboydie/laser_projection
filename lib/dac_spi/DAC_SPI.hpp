#pragma once

#include <Arduino.h>
#include <SPI.h>  // Add SPI library for control functions
#include "../utils/constants.h"
#include "../debug/Debug.hpp"

class DAC_SPI {
public:
    DAC_SPI(int _syncPin, int _ldacPin, int _clrPin);
    ~DAC_SPI() = default;
    void init();
    void deinit();
    void update(uint16_t A, uint16_t B);

private:
    void initSPIControl();
    void initDAC();
    
    void SPITransfer24(uint32_t frame);
    void sendSPICommand(uint32_t frame);

    const int syncPin;
    const int ldacPin;
    const int clrPin;
};