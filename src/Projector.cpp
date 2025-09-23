#include "Projector.h"
#include "FileManager.h"
#include "SystemState.h"

// Global instance
extern Projector projector;

// Implementation of Projector methods

void Projector::fillBufferDMA() {
    if (dmaActive || !fileManager || !fileManager->isPlaybackFileOpen()) return;
    
    // Get current position and file size
    uint32_t currentPos = fileManager->getCurrentPos();
    uint32_t fileSize = fileManager->getFileSize();
    
    // Position file at current read position
    if (!fileManager->prepareForDMARead(currentPos % fileSize)) return;

    // Calculate how many bytes to read
    size_t bytesToRead;
    if (fileSize - currentPos > BUFFER_SIZE * RECORD_SIZE) {
        bytesToRead = BUFFER_SIZE * RECORD_SIZE;
    } else {
        bytesToRead = fileSize - currentPos;
        fullBuffers++;
    }

    // Use SdFat's high-performance read (handles DMA internally)
    lastDMAStartTime = micros();
    
    // SdFat automatically uses DMA for large reads
    FsFile& file = fileManager->getPlaybackFile();
    size_t bytesRead = file.read(fillBuffer, bytesToRead);
    
    if (bytesRead == bytesToRead) {
        fileManager->setCurrentPos((currentPos + bytesToRead) % fileSize);
        dmaActive = false; // Read completed synchronously but efficiently
        
        // Update performance metrics
        dmaCompletionCount++;
        dmaFillTime = micros() - lastDMAStartTime;
    } else {
        Serial.println("ERROR: Failed to read buffer data");
        dmaActive = false;
    }
}

void Projector::rotateBuffers() {
    // Simple buffer rotation: every 20K points, rotate buffers regardless of DMA status
    // The three-buffer system ensures we always have a ready buffer available

    // Rotate buffer indices: active -> ready, ready -> fill, fill -> active
    oldActive = activeBuffer;
    oldReady = readyBuffer;
    oldFill = fillBuffer;
    
    activeBuffer = oldReady;  // Ready buffer becomes active
    readyBuffer = oldFill;    // Fill buffer becomes ready
    fillBuffer = oldActive;   // Old active buffer becomes fill target
    
    // Reset position in new active buffer
    activePos = 0;

    //Tell memory retreival to start filling the used-up buffer
    fullBuffers--;
    
    // Start filling the new fill buffer
    fillBufferDMA();
}

bool Projector::initializeBuffers() {
    if (!fileManager || !fileManager->isPlaybackFileOpen()) {
        Serial.println("ERROR: File manager not available or file not open");
        return false;
    }
    
    Serial.println("Initializing three-buffer system...");
    
    // Fill all three buffers sequentially
    fillBufferDMA();
    while (fullBuffers < 3) {
        delayMicroseconds(100);
        Serial.print("Waiting for buffers to fill: ");
        Serial.println(fullBuffers);
    }
    
    buffersInitialized = true;

    Serial.println("Three-buffer system initialized successfully");
    return true;
}