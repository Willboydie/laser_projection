#include "Projector.hpp"

constexpr uint32_t buffer_size = BUFFER_SIZE;

Projector::Projector(DAC_SPI& dacSPI, Laser& laser,SystemState& ss, RotatingBuffer& _buffer) 
: dacSPI(dacSPI)
, laser(laser)
, systemState(ss) 
, buffer(_buffer)
{}

void Projector::init() {
    Debug::msg("Initializing projector...");
    dacSPI.init();
    laser.init();
    Debug::msg("Projector initialized successfully");
    initialised = true;
}

void Projector::deinit() {
    Debug::msg("Deinitializing projector...");
    dacSPI.deinit();
    laser.deinit();
    Debug::msg("Projector deinitialized successfully");
    initialised = false;
}

void Projector::update() {
    if (!systemState.projecting) return;
    if (!initialised) {
        Debug::msg("ERROR: Projector not initialized");
        return;
    }
    
    uint32_t currentTime = micros();
    uint32_t elapsedTime = (currentTime >= lastUpdateTime) ? 
                            (currentTime - lastUpdateTime) : 
                            (0xFFFFFFFF - lastUpdateTime + currentTime + 1);
    
    if (elapsedTime >= delayUs) {
        
        uint16_t valA;
        uint16_t valB;
        buffer.updateValues(valA, valB, laserState);
        
        dacSPI.update(valA, valB);
        
        bufferIdx++;
        lastUpdateTime = currentTime;
        pointsProjected++;
        
        // Check if we need to rotate buffers
        if (bufferIdx >= buffer_size) {
            buffer.rotate();
            bufferIdx = 0;
        }
    }

    laser.update(laserState); // laser updated every loop to achieve pulsing (logic owned by Laser)
    return;
}