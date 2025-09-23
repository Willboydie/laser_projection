#include "Projector.h"
#include "FileManager.h"
#include "SystemState.h"

// Global instance
extern Projector projector;

// Implementation of Projector methods
void Projector::setupDMA() {
    // Configure DMA for SD card reading
    // Initialize with valid source buffer (will be reconfigured in startBufferFill)
    sdReadDMA.sourceBuffer((volatile void*)&IMXRT_USDHC1->DS_DATA, 1); // Use DS_DATA, not DS_ADDR
    sdReadDMA.destination(fillBuffer);
    sdReadDMA.triggerAtHardwareEvent(DMAMUX_SOURCE_SDHC);
    
    // Enable DMA interrupt
    attachInterruptVector(IRQ_DMA_CH0, dmaCompleteISR);
    NVIC_ENABLE_IRQ(IRQ_DMA_CH0);
}


// DMA completion interrupt handler
void dmaCompleteISR() {
    // Mark DMA as complete
    if (projector.getFullBuffers() >= 3) {
        projector.setDMAActive(false);
    } else {
        projector.fillBufferDMA();
    }

    projector.setDMACompletionCount(projector.getDMACompletionCount() + 1);
    projector.setDMAFillTime(micros() - projector.getLastDMAStartTime());
}


void Projector::fillBufferDMA() {
    if (dmaActive || !fileManager || !fileManager->isPlaybackFileOpen()) return;
    
    // Get current position and file size
    uint32_t currentPos = fileManager->getCurrentPos();
    uint32_t fileSize = fileManager->getFileSize();
    
    // Position file at current read position
    if (!fileManager->seekToPosition(currentPos % fileSize)) return;

    // Calculate how many bytes to read and configure DMA for this specific transfer
    size_t bytesToRead;
    if (fileSize - currentPos > BUFFER_SIZE * RECORD_SIZE) {
        bytesToRead = BUFFER_SIZE * RECORD_SIZE;
    } else {
        bytesToRead = fileSize - currentPos;
        fullBuffers++;
    }

    sdReadDMA.sourceBuffer((volatile void*)&IMXRT_USDHC1->DS_DATA, bytesToRead);
    sdReadDMA.destination(fillBuffer);

    fileManager->setCurrentPos((currentPos + bytesToRead) % fileSize);
    
    // Start DMA transfer
    lastDMAStartTime = micros();
    sdReadDMA.enable();
    dmaActive = true;
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