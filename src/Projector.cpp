#include "Projector.h"
#include "FileManager.h"
#include "SystemState.h"

// Global instance
extern Projector projector;

// Implementation of Projector methods

// We start with an empty buffer, and we must fill it.
// fillBuffer() is for filling a buffer.
// If
//  - the file has space for a whole buffer, we fill it in one move.
//  - the file has less remaining space than a whole buffer, we fill the buffer in 2 moves.

bool Projector::fillBuffer() {
    lastDMAStartTime = micros();
    size_t fileSize = fileManager->getFileSize();

    if (!fileManager) {
        Serial.println("ERROR: File manager not available");
        return false;
    }
    if (fileSize == 0) {
        Serial.println("ERROR: File size is 0");
        return false;
    }

    size_t bytesRead;
    if (fileSize - fileManager->getCurrentPos() > BUFFER_SIZE * RECORD_SIZE) {
        // We have space for a whole buffer
        bytesRead = do1BufferFill(fillingBuffer, BUFFER_SIZE * RECORD_SIZE);
        fileManager->seekToPosition(fileManager->getCurrentPos());
    } else { // We have less space than a whole buffer
        bytesRead = do1BufferFill(fillingBuffer, fileSize - fileManager->getCurrentPos());
        fileManager->seekToPosition(fileManager->getCurrentPos());
        bytesRead = do1BufferFill(fillingBuffer + bytesRead, BUFFER_SIZE * RECORD_SIZE - bytesRead);
        fileManager->seekToPosition(fileManager->getCurrentPos());
    }    
    return true;
}

int Projector::do1BufferFill(uint8_t* buffer, size_t bytesToRead) {

    // SdFat automatically uses DMA for large reads
    FsFile& file = fileManager->getPlaybackFile();
    if (!file) {
        Serial.println("ERROR: Failed to open file");
        return 0;
    }

    size_t bytesRead = file.read(buffer, bytesToRead);
    
    if (bytesRead == bytesToRead) {
        // We have read the whole buffer
        fileManager->setCurrentPos((fileManager->getCurrentPos() + bytesRead) % fileManager->getFileSize());

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
    fillPos = 0;
    
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
    
    // Fill bufferA (active) - manually set fillingBuffer pointer
    fillingBuffer = bufferA;
    fillPos = 0;
    if (!fillBuffer()) {
        Serial.println("ERROR: Failed to fill buffer A");
        return false;
    }
    Serial.println("Buffer A filled");
    
    // Fill bufferC (ready)
    fillingBuffer = bufferC;
    fillPos = 0;
    if (!fillBuffer()) {
        Serial.println("ERROR: Failed to fill buffer C");
        return false;
    }
    Serial.println("Buffer C filled");
    
    // Fill bufferB (filling)
    fillingBuffer = bufferB;
    fillPos = 0;
    if (!fillBuffer()) {
        Serial.println("ERROR: Failed to fill buffer B");
        return false;
    }
    Serial.println("Buffer B filled");
    
    buffersInitialized = true;
    dmaCompletionCount = 0;
    dmaFillTime = 0;
    lastDMAStartTime = 0;
    
    Serial.println("Three-buffer system initialized successfully");
    return true;
}