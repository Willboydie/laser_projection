#include "Projector.h"
#include "FileManager.h"
#include "SystemState.h"

// Global instance
extern Projector projector;

// Implementation of Projector methods

bool Projector::fillBuffer() {
    lastDMAStartTime = micros();

    if (dmaActive || !fileManager || !fileManager->openForRead()) return false;
    if (!fileManager->prepareForDMARead(0)) return false;

    uint32_t filePosition = 0;
    for (int i = 0; i < filesPerBuffer; i++) {
        int bytesRead = do1BufferFill(fillingBuffer, filePosition);
        if (bytesRead == 0) {
            Serial.println("ERROR: Failed to fill buffer");
            return false;
        }
        filePosition += bytesRead;
    }
    // Update performance metrics
    dmaCompletionCount++;
    dmaFillTime = micros() - lastDMAStartTime;
    return true;
}

int Projector::do1BufferFill(uint8_t* buffer, uint32_t filePosition) {
    if (!fileManager || !fileManager->openForRead()) return 0;
    if (!fileManager->prepareForDMARead(filePosition)) return 0;
    if (fileSize == 0) return 0;

    // Calculate how many bytes to read
    size_t bytesToRead;
    if (fileSize - filePosition > BUFFER_SIZE * RECORD_SIZE) {
        bytesToRead = BUFFER_SIZE * RECORD_SIZE;
    } else {
        bytesToRead = fileSize - filePosition;
    }
    
    // SdFat automatically uses DMA for large reads
    FsFile& file = fileManager->getPlaybackFile();
    size_t bytesRead = file.read(buffer, bytesToRead);
    
    if (bytesRead == bytesToRead) {
        fileManager->setCurrentPos((filePosition + bytesToRead) % fileManager->getFileSize());
        dmaActive = false; // Read completed synchronously but efficiently
        
        return bytesToRead;
    } else {
        Serial.println("ERROR: Failed to read buffer data");
        dmaActive = false;
        return 0;
    }
    
}

bool Projector::rotateBuffers() {
    // Simple buffer rotation: every 20K points, rotate buffers regardless of DMA status
    // The three-buffer system ensures we always have a ready buffer available

    // Rotate buffer indices: active -> ready, ready -> fill, fill -> active
    oldActive = activeBuffer;
    oldReady = readyBuffer;
    oldFill = fillingBuffer;
    
    activeBuffer = oldReady;  // Ready buffer becomes active
    readyBuffer = oldFill;    // Fill buffer becomes ready
    fillingBuffer = oldActive;   // Old active buffer becomes fill target
    
    // Reset position in new active buffer
    activePos = 0;
    
    // Start filling the new fill buffer
    if (!fillBuffer()) {
        Serial.println("ERROR: Failed to fill buffer");
        return false;
    }
    return true;
}

bool Projector::initializeBuffers() {
    if (!fileManager || !fileManager->openForRead()) {
        Serial.println("ERROR: File manager not available or file not open");
        return false;
    }
    
    Serial.println("Initializing three-buffer system...");
    
    // Fill all three buffers sequentially
    for (int i = 0; i < 3; i++) {
        if (!fillBuffer()) {
            Serial.print("ERROR: Failed to fill buffer: ");
            Serial.println(i);
            return false;
        }
    }
    
    buffersInitialized = true;
    dmaCompletionCount = 0;
    dmaFillTime = 0;
    lastDMAStartTime = 0;

    fileManager->closeAllFiles();
    Serial.println("Three-buffer system initialized successfully");
    return true;
}