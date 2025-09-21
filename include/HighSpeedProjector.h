#ifndef HIGHSPEEDPROJECTOR_H
#define HIGHSPEEDPROJECTOR_H

#include <Arduino.h>
#include <DMAChannel.h>
#include "DAC.h"

// Forward declaration
class FileManager;

// Configuration for 1MHz projection with three-buffer system
#define BUFFER_SIZE 20000  // 20,000 points = 100KB per buffer
#define RECORD_SIZE 5
#define NUM_BUFFERS 3

// Three-buffer circular system
class HighSpeedProjector {
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
    uint8_t* buffers[NUM_BUFFERS] = {bufferA, bufferB, bufferC};
    uint8_t activeBufferIndex = 0;    // Currently projecting from this buffer
    uint8_t fillBufferIndex = 1;      // Currently filling this buffer
    uint8_t readyBufferIndex = 2;     // This buffer is ready to become active
    
    uint32_t activePos = 0;           // Position within active buffer
    bool buffersInitialized = false;  // All buffers filled on startup
    bool fillingBuffer = false;       // DMA is currently filling a buffer
    
    // DMA for background file operations
    DMAChannel sdReadDMA;
    bool dmaActive = false;
    
    // Timing
    uint32_t lastUpdateTime = 0;
    uint32_t currentDelayUs = 0;
    
    // Laser State
    bool laser_on = true;
    
    // File manager reference
    FileManager* fileManager;
    
    // Performance monitoring
    uint32_t lastDMAStartTime = 0;
    uint32_t totalDMATransfers = 0;
    uint32_t totalDMATime = 0;
    uint32_t lastPerformanceReport = 0;

public:
    void init(FileManager* fm) {
        fileManager = fm;
        // Initialize DMA for SD card operations
        setupDMA();
        
        // Initialize buffer state
        activePos = 0;
        buffersInitialized = false;
        fillingBuffer = false;
        
        // Initialize performance monitoring
        totalDMATransfers = 0;
        totalDMATime = 0;
        lastPerformanceReport = millis();
        
        Serial.println("High-speed projector initialized");
    }
    
    void setupDMA();
    
    bool initializeBuffers() {
        if (!fileManager || !fileManager->isPlaybackFileOpen()) {
            Serial.println("ERROR: File manager not available or file not open");
            return false;
        }
        
        Serial.println("Initializing three-buffer system...");
        
        // Fill all three buffers sequentially
        for (int i = 0; i < NUM_BUFFERS; i++) {
            if (!fillBufferFromFile(buffers[i], i * BUFFER_SIZE * RECORD_SIZE)) {
                Serial.println("ERROR: Failed to initialize buffers");
                return false;
            }
        }
        
        buffersInitialized = true;
        Serial.println("Three-buffer system initialized successfully");
        return true;
    }
    
    bool startProjection(uint32_t delayUs) {
        if (!buffersInitialized) {
            Serial.println("ERROR: Buffers not initialized");
            return false;
        }
        
        currentDelayUs = delayUs;
        isProjecting = true;
        
        Serial.print("Projection started: ");
        Serial.print(fileManager->getFileSize() / RECORD_SIZE);
        Serial.println(" records");
        return true;
    }
    
    void stopProjection() {
        isProjecting = false;
        Serial.println("Projection stopped");
    }
    
    bool resumeProjection() {
        if (!fileManager || !fileManager->isPlaybackFileOpen()) {
            return false;
        }
        isProjecting = true;
        return true;
    }
    
    // Buffer filling methods
    bool fillBufferFromFile(uint8_t* buffer, uint32_t filePosition);
    void fillBuffer();
    
    // Buffer rotation methods
    void rotateBuffers();
    
    // Main projection loop - optimized for 1MHz
    void update() {
        if (!isProjecting || !buffersInitialized) return;
        
        uint32_t currentTime = micros();
        uint32_t elapsedTime = (currentTime >= lastUpdateTime) ? 
                              (currentTime - lastUpdateTime) : 
                              (0xFFFFFFFF - lastUpdateTime + currentTime + 1);
        
        if (elapsedTime >= currentDelayUs) {
            // Get data from active buffer (optimized memory access)
            uint8_t* activeBuffer = buffers[activeBufferIndex];
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
    float getProgress() const;
};

// Global instance
extern HighSpeedProjector highSpeedProjector;

#endif // HIGHSPEEDPROJECTOR_H


