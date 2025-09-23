#ifndef HIGHSPEEDPROJECTOR_H
#define HIGHSPEEDPROJECTOR_H

#include <Arduino.h>
#include <DMAChannel.h>
#include "DAC.h"
#include "SystemState.h"
#include "FileManager.h"
#include "TTL.h"

// Forward declaration
class FileManager;
class SystemState;

// Configuration for 1MHz projection with three-buffer system
#define BUFFER_SIZE 20000  // 20,000 points = 100KB per buffer
#define RECORD_SIZE 5

// Three-buffer circular system
class Projector {
private:
    // Three buffers in DTCM for maximum performance (100KB each)
    __attribute__((aligned(32))) 
    __attribute__((section(".dtcm")))
    uint8_t bufferA[BUFFER_SIZE * RECORD_SIZE];
    
    __attribute__((aligned(32))) 
    __attribute__((section(".dtcm")))
    uint8_t bufferB[BUFFER_SIZE * RECORD_SIZE];
    
    __attribute__((aligned(32))) 
    __attribute__((section(".dtcm")))
    uint8_t bufferC[BUFFER_SIZE * RECORD_SIZE];
    
    // Buffer management
    uint8_t* activeBuffer = bufferA;    // Currently projecting from this buffer
    uint8_t* fillBuffer = bufferB;      // Currently filling this buffer  
    uint8_t* readyBuffer = bufferC;     // This buffer is ready to become active
    uint8_t* oldActive;
    uint8_t* oldReady;
    uint8_t* oldFill;
    uint8_t fullBuffers = 0;

    uint32_t activePos = 0;           // Position within active buffer
    bool buffersInitialized = false;  // All buffers filled on startup
    
    // DMA for background file operations
    DMAChannel sdReadDMA;
    bool dmaActive = false;
    uint32_t dmaCompletionCount = 0;
    uint32_t dmaFillTime = 0;
    uint32_t lastDMAStartTime = 0;
    
    // Projection Timing
    uint32_t lastUpdateTime = 0;
    uint32_t currentDelayUs = 0;
    
    // Laser State
    bool laser_on = true;
    
    // File manager reference
    FileManager* fileManager;
    SystemState* systemState;

public:

    void init(FileManager* fm, SystemState* ss) {
        fileManager = fm;
        systemState = ss;
        // Initialize DMA for SD card operations
        setupDMA();
        
        // Initialize buffer state
        activePos = 0;
        buffersInitialized = false;
        fullBuffers = 0;
        
        // Initialize performance monitoring
        dmaCompletionCount = 0;
        dmaFillTime = 0;
        lastDMAStartTime = 0;
        
        Serial.println("High-speed projector initialized");
    }
    
    void setupDMA();
    
    bool initializeBuffers();
    
    bool startProjection(uint32_t delayUs) {
        if (!buffersInitialized) {
            Serial.println("ERROR: Buffers not initialized");
            return false;
        }
        
        currentDelayUs = delayUs;
        systemState->setProjecting(true);
        
        Serial.print("Projection started: ");
        Serial.print(fileManager->getFileSize() / RECORD_SIZE);
        Serial.println(" records");
        return true;
    }
    
    void stopProjection() {
        systemState->setProjecting(false);
        Serial.println("Projection stopped");
    }
    
    bool resumeProjection() {
        if (!fileManager || !fileManager->isPlaybackFileOpen()) {
            return false;
        }
        systemState->setProjecting(true);
        return true;
    }
    
    // Buffer filling methods
    bool fillBufferFromFile(uint8_t* buffer, uint32_t filePosition);
    void fillBufferDMA();
    
    // Buffer rotation methods
    void rotateBuffers();
    
    // Main projection loop - optimized for 1MHz
    void update() {
        if (!systemState->isCurrentlyProjecting() || !buffersInitialized) return;
        
        uint32_t currentTime = micros();
        uint32_t elapsedTime = (currentTime >= lastUpdateTime) ? 
                              (currentTime - lastUpdateTime) : 
                              (0xFFFFFFFF - lastUpdateTime + currentTime + 1);
        
        if (elapsedTime >= currentDelayUs) {
            // Get data from active buffer (optimized memory access)
            uint32_t offset = (activePos << 2) + activePos; // *5 without multiplication
            uint16_t valA = (activeBuffer[offset] << 8) | activeBuffer[offset + 1];
            uint16_t valB = (activeBuffer[offset + 2] << 8) | activeBuffer[offset + 3];
            uint8_t laserState = activeBuffer[offset + 4];
            
            // Update DAC (ultra-fast inline macro)
            FAST_DAC_UPDATE(valA, valB);
            
            // Update laser (direct register access)
            if (laser_on != laserState) {
                if (laserState) {
                    LASER_ON_FAST();
                } else {
                    LASER_OFF_FAST();
                }
                laser_on = laserState;
            }
            
            activePos++;
            lastUpdateTime = currentTime;
            
            // Check if we need to rotate buffers
            if (activePos >= BUFFER_SIZE) {
                rotateBuffers();
            }
        }
    }
    
    // Getters for status
    uint32_t getActivePosition() const { return activePos; }
    uint32_t getDMAFillTime() const { return dmaFillTime; }
    uint32_t getDMACompletionCount() const { return dmaCompletionCount; }
    uint32_t getLastDMAStartTime() const { return lastDMAStartTime; }
    uint8_t getFullBuffers() const { return fullBuffers; }

    // Setters
    void setDMAActive(bool active) { dmaActive = active; }
    void setDMACompletionCount(uint32_t count) { dmaCompletionCount = count; }
    void setDMAFillTime(uint32_t time) { dmaFillTime = time; }
};

    // Global instance
    extern Projector projector;

#endif // HIGHSPEEDPROJECTOR_H


