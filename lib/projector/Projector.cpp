#include "Projector.hpp"

constexpr uint32_t buffer_size = BUFFER_SIZE;

Projector* Projector::instance = nullptr;

Projector::Projector(DAC_SPI& dacSPI, Laser& laser, SystemState& ss, RotatingBuffer& _buffer) 
: dacSPI(dacSPI)
, laser(laser)
, systemState(ss) 
, buffer(_buffer)
{}

void Projector::init() {
    Debug::msg("Initializing projector...");
    dacSPI.init();
    laser.init();
    instance = this;
    Debug::msg("Projector initialized successfully");
    initialised = true;
}

void Projector::deinit() {
    Debug::msg("Deinitializing projector...");
    stop();
    dacSPI.deinit();
    laser.deinit();
    instance = nullptr;
    Debug::msg("Projector deinitialized successfully");
    initialised = false;
}

void Projector::start() {
    if (!initialised || running) return;

    uint32_t period = (delayUs < MIN_POINT_PERIOD_US) ? MIN_POINT_PERIOD_US : delayUs;
    timer.begin(isrTrampoline, period);
    running = true;
    Debug::msg("Projection ISR started, period=");
    Debug::msg(period);
    Debug::msg("us");
}

void Projector::stop() {
    if (!running) return;
    timer.end();
    running = false;
    laser.off();
    laserState = false;
}

void Projector::setDelay(uint32_t _delayUs) {
    delayUs = _delayUs;
    if (running) {
        uint32_t period = (delayUs < MIN_POINT_PERIOD_US) ? MIN_POINT_PERIOD_US : delayUs;
        timer.update(period);
    }
}

void Projector::isrTrampoline() {
    if (instance) instance->isr();
}

void Projector::isr() {
    uint16_t valA = 0;
    uint16_t valB = 0;
    bool laserVal = laserState;

    if (systemState.projectMode == PROJECT_MODE_POINT) {
        valA = setPointX;
        valB = setPointY;
        laserVal = true;
    } else {
        buffer.updateValues(valA, valB, laserVal);
    }
    laserState = laserVal;

    dacSPI.update(valA, valB);
    laser.update(laserState);
    pointsProjected++;

    if (systemState.projectMode == PROJECT_MODE_PATH) {
        if (buffer.bufferIdx >= buffer_size) {
            buffer.rotate();
        }
    }
}
