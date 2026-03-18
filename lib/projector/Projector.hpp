#pragma once

#include <Arduino.h>
#include "../system_state/SystemState.hpp"
#include "../file_manager/FileManager.hpp"
#include "../laser/Laser.hpp"
#include "../utils/constants.h"
#include "../dac_spi/DAC_SPI.hpp"
#include "../buffer/RotatingBuffer.hpp"
#include "../debug/Debug.hpp"
#include "../loopback/LoopBack.hpp"


// Three-buffer circular system
class Projector {
public:
    Projector(DAC_SPI& dacSPI, Laser& laser, SystemState& ss, RotatingBuffer& _buffer, LoopBack& _loopBack);
    ~Projector() = default;
    void init();
    void deinit();
    void update();

    void toggleLoopBack() { loopBack.toggle(); }

    uint32_t getPointsProjected() const { return pointsProjected; }
    uint32_t getDelay() const { return delayUs; }
    void setPointsProjected(uint32_t points) { pointsProjected = points; }
    void setDelay(uint32_t _delayUs) { delayUs = _delayUs; }

    void setSetPointX(uint16_t _setPointX) { setPointX = _setPointX; }
    void setSetPointY(uint16_t _setPointY) { setPointY = _setPointY; }

private:
    DAC_SPI& dacSPI;
    Laser& laser;
    SystemState& systemState;
    RotatingBuffer& buffer;
    LoopBack& loopBack;

    // for POINT mode (where specific points are commanded)
    uint16_t setPointX = 0;
    uint16_t setPointY = 0;

    uint32_t bufferIdx = 0;
    
    uint32_t lastUpdateTime = 0;
    uint32_t delayUs = 0;
    uint32_t pointsProjected = 0;

    bool laserState = false;
    bool initialised = false;
};