#pragma once

#include <Arduino.h>
#include "../system_stateSystemState.hpp"
#include "../file_manager/FileManager.hpp"
#include "../laser_control/TTL.hpp"
#include "../utils/constants.h"
#include "../dac_spi/DAC_SPI.hpp"

// Three-buffer circular system
class Projector {
public:
    Projector(FileManager& fm, SystemState& ss, DAC_SPI& dacSPI, Laser& laser);
    ~Projector() = default;
    void init();
    void reset();
    void update();

private:
    SystemState& systemState;
    FileManager& fileManager;
    DAC_SPI& dacSPI;
    Laser& laser;

    bool initializeBuffers();
    bool startProjection(float delayUs);
    bool resumeProjection();
    bool fillBuffer();
    int do1BufferFill(uint8_t* buffer, size_t bytesToRead);
    bool rotateBuffers();
    
    uint32_t getActivePosition() const { return activePos; }
    uint32_t getFillPosition() const { return fillPos; }
    uint32_t getDMAFillTime() const { return dmaFillTime; }
    uint32_t getDMACompletionCount() const { return dmaCompletionCount; }
    uint32_t getLastDMAStartTime() const { return lastDMAStartTime; }
    uint32_t getPointsProjected() const { return pointsProjected; }

    void setDMACompletionCount(uint32_t count) { dmaCompletionCount = count; }
    void setFillPosition(uint32_t position) { fillPos = position; }
    void setDMAFillTime(uint32_t time) { dmaFillTime = time; }
    void setLastDMAStartTime(uint32_t time) { lastDMAStartTime = time; }
    void setPointsProjected(uint32_t points) { pointsProjected = points; }
    
    // Three buffers aligned to 4 bytes (as required by SdFat)
    __attribute__((aligned(4))) 
    uint8_t bufferA[BUFFER_SIZE * RECORD_SIZE];
    
    __attribute__((aligned(4))) 
    uint8_t bufferB[BUFFER_SIZE * RECORD_SIZE];
    
    __attribute__((aligned(4))) 
    uint8_t bufferC[BUFFER_SIZE * RECORD_SIZE];
    
    // Buffer management
    uint8_t* activeBuffer = bufferA;    // Currently projecting from this buffer
    uint8_t* fillingBuffer = bufferB;      // Currently filling this buffer  
    uint8_t* readyBuffer = bufferC;     // This buffer is ready to become active
    uint8_t* oldActive;
    uint8_t* oldReady;
    uint8_t* oldFill;

    uint32_t activePos = 0;           // Position within active buffer
    uint32_t fillPos = 0;
    bool buffersInitialized = false;  // All buffers filled on startup
    
    // File reading state (SdFat handles DMA internally)
    bool dmaActive = false;
    uint32_t dmaCompletionCount = 0;
    uint32_t dmaFillTime = 0;
    uint32_t lastDMAStartTime = 0;
    
    // Projection Timing
    uint32_t lastUpdateTime = 0;
    uint32_t currentDelayUs = 0;
    uint32_t pointsProjected = 0;
    
    // Laser State
    bool laser_on = true;
    
    int filesPerBuffer = 0;
};


