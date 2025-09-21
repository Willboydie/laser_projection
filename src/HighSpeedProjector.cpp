#include "HighSpeedProjector.h"
#include "FileManager.h"

// Global instance
HighSpeedProjector highSpeedProjector;

// DMA completion interrupt handler
void dmaCompleteISR() {
    // Mark DMA as complete - performance tracking happens in rotateBuffers()
    highSpeedProjector.dmaActive = false;
}

// Implementation of HighSpeedProjector methods
void HighSpeedProjector::setupDMA() {
    // Configure DMA for SD card reading
    // Initialize with valid source buffer (will be reconfigured in startBufferFill)
    sdReadDMA.sourceBuffer((volatile void*)&IMXRT_SDHC1->DS_DATA, 1); // Use DS_DATA, not DS_ADDR
    sdReadDMA.destination(fillBuffer);
    sdReadDMA.triggerAtHardwareEvent(DMAMUX_SOURCE_SDHC);
    
    // Enable DMA interrupt
    attachInterruptVector(IRQ_DMA_CH0, dmaCompleteISR);
    NVIC_ENABLE_IRQ(IRQ_DMA_CH0);
}

bool HighSpeedProjector::fillBufferFromFile(uint8_t* buffer, uint32_t filePosition) {
    if (!fileManager || !fileManager->isPlaybackFileOpen()) {
        return false;
    }
    
    uint32_t fileSize = fileManager->getFileSize();
    uint32_t seekPos = filePosition % fileSize;
    
    // Position file at the correct location
    if (!fileManager->seekToPosition(seekPos)) {
        return false;
    }
    
    // Read entire buffer from file
    size_t bytesToRead = BUFFER_SIZE * RECORD_SIZE;
    size_t bytesRead = fileManager->getPlaybackFile().read(buffer, bytesToRead);
    
    if (bytesRead != bytesToRead) {
        Serial.print("WARNING: Only read ");
        Serial.print(bytesRead);
        Serial.print(" of ");
        Serial.print(bytesToRead);
        Serial.println(" bytes");
    }
    
    return true;
}

void HighSpeedProjector::fillBuffer() {
    if (fillingBuffer || !fileManager || !fileManager->isPlaybackFileOpen()) return;
    
    fillingBuffer = true;
    
    // Get current position and file size
    uint32_t currentPos = fileManager->getBytesRead();
    uint32_t fileSize = fileManager->getFileSize();
    
    // Position file at current read position
    if (!fileManager->seekToPosition(currentPos % fileSize)) {
        fillingBuffer = false;
        return;
    }
    
    // Calculate how many bytes to read
    size_t bytesToRead = BUFFER_SIZE * RECORD_SIZE;
    
    // Configure DMA for this specific transfer
    sdReadDMA.sourceBuffer((volatile void*)&IMXRT_SDHC1->DS_DATA, bytesToRead);
    sdReadDMA.destination(buffers[fillBufferIndex]);
    
    // Start DMA transfer
    lastDMAStartTime = micros();
    sdReadDMA.enable();
    dmaActive = true;
}


void HighSpeedProjector::rotateBuffers() {
    // Simple buffer rotation: every 20K points, rotate buffers regardless of DMA status
    // The three-buffer system ensures we always have a ready buffer available
    
    // Performance monitoring - check if previous DMA transfer completed
    if (fillingBuffer && !dmaActive) {
        // DMA transfer completed successfully
        uint32_t currentPos = fileManager->getBytesRead();
        uint32_t fileSize = fileManager->getFileSize();
        size_t bytesTransferred = BUFFER_SIZE * RECORD_SIZE;
        
        // Performance tracking
        uint32_t dmaTime = micros() - lastDMAStartTime;
        totalDMATransfers++;
        totalDMATime += dmaTime;
        
        // Report average buffer fill time every 10 seconds
        uint32_t currentTime = millis();
        if (currentTime - lastPerformanceReport >= 10000) {
            uint32_t avgFillTime = totalDMATime / totalDMATransfers;
            Serial.print("Average buffer fill time: ");
            Serial.print(avgFillTime);
            Serial.print("μs (");
            Serial.print((BUFFER_SIZE * RECORD_SIZE * 1000) / avgFillTime);
            Serial.println(" KB/s)");
            
            // Reset counters
            totalDMATransfers = 0;
            totalDMATime = 0;
            lastPerformanceReport = currentTime;
        }
        
        // Update file manager's read position
        fileManager->setBytesRead((currentPos + bytesTransferred) % fileSize);
        
        fillingBuffer = false;
    }
    
    // Rotate buffer indices: active -> ready, ready -> fill, fill -> active
    uint8_t oldActive = activeBufferIndex;
    uint8_t oldReady = readyBufferIndex;
    uint8_t oldFill = fillBufferIndex;
    
    activeBufferIndex = oldReady;  // Ready buffer becomes active
    readyBufferIndex = oldFill;    // Fill buffer becomes ready
    fillBufferIndex = oldActive;   // Old active buffer becomes fill target
    
    // Reset position in new active buffer
    activePos = 0;
    
    // Start filling the new fill buffer
    fillBuffer();
}


float HighSpeedProjector::getProgress() const {
    if (!fileManager) return 0.0f;
    return fileManager->getReadProgress();
}
