#pragma once

#include <Arduino.h>
#include "../system_state/SystemState.hpp"
#include "../file_manager/FileManager.hpp"
#include "../laser/Laser.hpp"
#include "../utils/constants.h"
#include "../dac_spi/DAC_SPI.hpp"
#include "../buffer/RotatingBuffer.hpp"
#include "../debug/Debug.hpp"

#define MIN_POINT_PERIOD_US 10

class Projector {
public:
    Projector(DAC_SPI& dacSPI, Laser& laser, SystemState& ss, RotatingBuffer& _buffer);
    ~Projector() = default;
    void init();
    void deinit();

    void start();
    void stop();

    uint32_t getPointsProjected() const { return pointsProjected; }
    uint32_t getDelay() const { return delayUs; }
    void setPointsProjected(uint32_t points) { pointsProjected = points; }
    void setDelay(uint32_t _delayUs);

    void setSetPointX(uint16_t _setPointX) { setPointX = _setPointX; }
    void setSetPointY(uint16_t _setPointY) { setPointY = _setPointY; }

    bool isRunning() const { return running; }

private:
    static void isrTrampoline();
    void isr();

    static Projector* instance;

    DAC_SPI& dacSPI;
    Laser& laser;
    SystemState& systemState;
    RotatingBuffer& buffer;
    IntervalTimer timer;

    uint16_t setPointX = 0;
    uint16_t setPointY = 0;

    volatile uint32_t pointsProjected = 0;
    volatile bool laserState = false;

    uint32_t delayUs = 0;
    bool initialised = false;
    bool running = false;
};
