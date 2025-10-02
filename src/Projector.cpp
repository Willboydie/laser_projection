#include "Projector.h"
#include "FileManager.h"
#include "SystemState.h"

// Global instance
extern Projector projector;

// Implementation of Projector methods

bool Projector::fillBuffer() {
    lastDMAStartTime = micros();
    size_t fileSize = fileManager->getFileSize();
    if (fileSize == 0) return false;

    for (int i = 0; i < filesPerBuffer; i++) {
        fileManager->seekToPosition(fileManager->getCurrentPos());
        int bytesRead = do1BufferFill(fillingBuffer, fileSize, fileManager->getCurrentPos());
        if (bytesRead == 0) {
            Serial.println("ERROR: Failed to fill buffer");
            return false;
        }
        fileManager->setCurrentPos(fileManager->getCurrentPos() + bytesRead);
    }
    
    return true;
}

int Projector::do1BufferFill(uint8_t* buffer, size_t fileSize, uint32_t filePosition) {
    if (!fileManager) return 0;
    if (fileSize == 0) return 0;

    // SdFat automatically uses DMA for large reads
    FsFile& file = fileManager->getPlaybackFile();
    if (!file) {
        Serial.println("ERROR: Failed to open file");
        return 0;
    }
    
    // Calculate how many bytes to read
    size_t bytesToRead;
    if (fileSize - filePosition > BUFFER_SIZE * RECORD_SIZE) {
        bytesToRead = BUFFER_SIZE * RECORD_SIZE;
    } else {
        bytesToRead = fileSize - filePosition;
    }
    
    
    size_t bytesRead = file.read(buffer, bytesToRead);
    
    if (bytesRead == bytesToRead) {
        // Update performance metrics
        dmaCompletionCount++;
        dmaFillTime = micros() - lastDMAStartTime;
        return bytesRead;

    } else {
        Serial.println("ERROR: Failed to read buffer data");
        return 0;
    }
}


bool Projector::rotateBuffers() {
    // Simple buffer rotation: every BUFFER_SIZE points, rotate buffers
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
    if (!fileManager) {
        Serial.println("ERROR: File manager not available or file not open");
        return false;
    }

    fileManager->closeAllFiles();

    // Update file size and open for reading
    if (fileManager->openForRead()) {
        fileManager->setCurrentPos(0);
        fileManager->seekToPosition(0);
        Serial.print("SD file opened for read: ");
        Serial.print(fileManager->getFileSize());
        Serial.println(" bytes");
    } else {
        Serial.println("ERROR: Failed to prepare for buffer fill. Buffers not initialized.");
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