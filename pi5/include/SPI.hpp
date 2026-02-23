#pragma once

#include "Constants.hpp"

class SPI {
public:
    bool init();
    bool updateDAC(uint16_t A, uint16_t B);

private:
    void configureSPI();
    void transfer24(uint8_t cmd, uint16_t value);
    // spidev functions

}