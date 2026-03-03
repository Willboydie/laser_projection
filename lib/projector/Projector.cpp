#include "Projector.hpp"


constexpr uint32_t buffer_size = BUFFER_SIZE;

Projector::Projector(DAC_SPI& dacSPI, Laser& laser,SystemState& ss, RotatingBuffer& _buffer) 
: dacSPI(dacSPI)
, laser(laser)
, systemState(ss) 
, buffer(_buffer)
{}

void Projector::init() {
    Serial.println("Initializing projector...");
    dacSPI.init();
    laser.init();
    Serial.println("Projector initialized successfully");
    initialised = true;
}

void Projector::deinit() {
    Serial.println("Deinitializing projector...");
    dacSPI.deinit();
    laser.deinit();
    Serial.println("Projector deinitialized successfully");
    initialised = false;
}


void Projector::update() {
    if (!systemState.projecting) return;
    if (!initialised) {
        Serial.println("ERROR: Projector not initialized");
        return;
    }
    
    uint32_t currentTime = micros();
    uint32_t elapsedTime = (currentTime >= lastUpdateTime) ? 
                            (currentTime - lastUpdateTime) : 
                            (0xFFFFFFFF - lastUpdateTime + currentTime + 1);
    
    if (elapsedTime >= delayUs) {
        
        uint32_t offset;
        uint16_t valA;
        uint16_t valB;
        uint8_t laserState;
        buffer.updateValues(&offset, &valA, &valB, &laserState);

        laser.update(&laserState);
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
    return;
}