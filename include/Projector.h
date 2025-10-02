#ifndef HIGHSPEEDPROJECTOR_H
#define HIGHSPEEDPROJECTOR_H

#include <Arduino.h>
#include "DAC.h"
#include "SystemState.h"
#include "FileManager.h"
#include "TTL.h"
#include "Constants.h"

// Forward declaration
class FileManager;
class SystemState;

// Three-buffer circular system
class Projector {
private:
    // Three buffers aligned to 4 bytes (as required by SdFat)
    __attribute__((aligned(4))) 
    // __attribute__((section(".dtcm")))
    uint8_t bufferA[BUFFER_SIZE * RECORD_SIZE];
    
    __attribute__((aligned(4))) 
    // __attribute__((section(".dtcm")))
    uint8_t bufferB[BUFFER_SIZE * RECORD_SIZE];
    
    __attribute__((aligned(4))) 
    // __attribute__((section(".dtcm")))   
    uint8_t bufferC[BUFFER_SIZE * RECORD_SIZE];
    
    // Buffer management
    uint8_t* activeBuffer = bufferA;    // Currently projecting from this buffer
    uint8_t* fillingBuffer = bufferB;      // Currently filling this buffer  
    uint8_t* readyBuffer = bufferC;     // This buffer is ready to become active
    uint8_t* oldActive;
    uint8_t* oldReady;
    uint8_t* oldFill;

    uint32_t activePos = 0;           // Position within active buffer
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
    
    // File manager reference
    FileManager* fileManager;
    SystemState* systemState;

    int filesPerBuffer = 0;

public:

    void init(FileManager* fm, SystemState* ss) {
        fileManager = fm;
        systemState = ss;
        // SdFat handles DMA initialization internally
        
        // Initialize buffer state
        activePos = 0;
        buffersInitialized = false;
        
        // Initialize performance monitoring
        dmaCompletionCount = 0;
        dmaFillTime = 0;
        lastDMAStartTime = 0;

        filesPerBuffer = (BUFFER_SIZE * RECORD_SIZE + (fileManager->getFileSize() - 1)) / (fileManager->getFileSize());
        
        Serial.println("High-speed projector initialized with SdFat");
    }
    
    bool initializeBuffers();
    
    bool startProjection(float delayUs) {
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
    bool fillBuffer();
    int do1BufferFill(uint8_t* buffer, size_t fileSize, uint32_t filePosition);
    
    // Buffer rotation methods
    bool rotateBuffers();
    
    // Main projection loop - optimized for 1MHz
    bool update() {
        if (!systemState->isCurrentlyProjecting() || !buffersInitialized) return false;
        
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
            // Serial.print("DAC A: ");
            // Serial.println(valA);
            // Serial.print("DAC B: ");
            // Serial.println(valB);
            // Serial.print("Laser State: ");
            // Serial.println(laserState);
            // Serial.println("--------------------------------");
            
            // Update laser (direct register access)
            if (laser_on != laserState) {
                if (laserState) {
                    digitalWriteFast(LASER_ON, LOW);
                } else {
                    digitalWriteFast(LASER_ON, HIGH);
                }
                laser_on = laserState;
            }
            
            activePos++;
            lastUpdateTime = currentTime;
            pointsProjected++;
            
            // Check if we need to rotate buffers
            if (activePos >= BUFFER_SIZE) {
                if (!rotateBuffers()) {
                    Serial.println("ERROR: Failed to rotate buffers");
                    return false;
                }
            }
        }
        return true;
    }   
    
    // Getters for status
    uint32_t getActivePosition() const { return activePos; }
    uint32_t getDMAFillTime() const { return dmaFillTime; }
    uint32_t getDMACompletionCount() const { return dmaCompletionCount; }
    uint32_t getLastDMAStartTime() const { return lastDMAStartTime; }
    uint32_t getPointsProjected() const { return pointsProjected; }

    // Setters
    void setDMACompletionCount(uint32_t count) { dmaCompletionCount = count; }
    void setDMAFillTime(uint32_t time) { dmaFillTime = time; }
    void setLastDMAStartTime(uint32_t time) { lastDMAStartTime = time; }
    void setPointsProjected(uint32_t points) { pointsProjected = points; }
};

    // Global instance
    extern Projector projector;

#endif // HIGHSPEEDPROJECTOR_H


