#include "CommandProcessor.h"
#include "DAC.h"
#include "Projector.h"
#include "TTL.h"
#include "FileManager.h"
#include "SystemState.h"

CommandProcessor::CommandProcessor(FileManager* fm, SystemState* ss, Projector* proj) 
    : fileManager(fm), systemState(ss), projector(proj) {
    // Constructor - just store pointers
}

void CommandProcessor::processCommand(uint8_t cmd, uint8_t payload[RECORD_SIZE]) {
    switch (cmd) {
        case WRITE_CMD:
            printCommandInfo(cmd, "WRITE");
            handleWriteCommand();
            break;
            
        case PROJECT_CMD:
            printCommandInfo(cmd, "PROJECT");
            handleProjectCommand();
            break;
            
        case SAVE_CMD:
            printCommandInfo(cmd, "SAVE");
            handleSaveCommand(payload);
            break;
            
        case PLAY_CMD:
            printCommandInfo(cmd, "PLAY");
            handlePlayCommand(payload);
            break;
            
        case PAUSE_CMD:
            printCommandInfo(cmd, "PAUSE");
            handlePauseCommand();
            break;
            
        case RESET_CMD:
            printCommandInfo(cmd, "RESET");
            handleResetCommand();
            break;
            
        default:
            Serial.print("ERROR: Unknown command 0b");
            Serial.println(cmd, BIN);
    }
}

void CommandProcessor::handleWriteCommand() {
    // Close any open files
    fileManager->closeAllFiles();
    
    // Wipe file and open for writing
    if (fileManager->wipeFile() && fileManager->openForWrite()) {
        systemState->setMode(SystemState::MODE_WRITE);
        systemState->setProjecting(false);
        fileManager->setCurrentPos(0);
        fileManager->seekToPosition(0);
        Serial.println("Ready to save records");
    } else {
        Serial.println("ERROR: Failed to prepare for writing");
    }
}

void CommandProcessor::handleProjectCommand() {
    // Close any open files
    fileManager->closeAllFiles();
    
    // Update file size and open for reading
    if (fileManager->updateFileSize() && fileManager->openForRead()) {
        systemState->setMode(SystemState::MODE_PROJECT);
        systemState->setProjecting(false);
        fileManager->setCurrentPos(0);
        fileManager->seekToPosition(0);
        Serial.println("Ready for playback");
    } else {
        Serial.println("ERROR: Failed to prepare for projection");
    }
}

void CommandProcessor::handleSaveCommand(uint8_t payload[RECORD_SIZE]) {
    if (systemState->getMode() != SystemState::MODE_WRITE) {
        Serial.println("ERROR: Not in WRITE mode");
        return;
    }
    
    if (!fileManager->writeRecord(payload)) {
       Serial.println("ERROR: Failed to save record");
       return;
    }
}

void CommandProcessor::handlePlayCommand(uint8_t payload[RECORD_SIZE]) {
    if (systemState->getMode() != SystemState::MODE_PROJECT) {
        Serial.println("ERROR: Not in PROJECT mode");
        return;
    }
    
    // Check file size
    if (fileManager->getFileSize() < RECORD_SIZE) {
        Serial.println("ERROR: File too small for playback");
        return;
    }
    
    // Calculate delay
    uint32_t delayUs = calculateDelayUs(payload);
    systemState->setDelay(delayUs);
    
    // Initialize buffers first, then start projection
    if (projector->initializeBuffers()) {
        if (projector->startProjection(delayUs)) {
            systemState->setProjecting(true);
            Serial.print("HIGH-SPEED PLAYBACK: Started with ");
            Serial.print(delayUs);
            Serial.println("μs delay");
        } else {
            Serial.println("ERROR: Failed to start projection");
        }
    } else {
        Serial.println("ERROR: Failed to initialize buffers");
    }
}

void CommandProcessor::handlePauseCommand() {
    if (systemState->getMode() != SystemState::MODE_PROJECT) {
        Serial.println("ERROR: Not in PROJECT mode");
        return;
    }
    
    bool currentlyProjecting = systemState->isCurrentlyProjecting();
    
    if (currentlyProjecting) {
        projector->stopProjection();
        systemState->setProjecting(false);
        Serial.println("HIGH-SPEED PLAYBACK: Paused");
    } else {
        if (projector->resumeProjection()) {
            systemState->setProjecting(true);
            Serial.println("HIGH-SPEED PLAYBACK: Resumed");
        } else {
            Serial.println("ERROR: Failed to resume projection");
        }
    }
}

void CommandProcessor::handleResetCommand() {
    Serial.println("Initiating Reset...");
    
    // Stop projection
    projector->stopProjection();
    systemState->setProjecting(false);
    
    // Close all files
    fileManager->closeAllFiles();

    // Initialize file system
    if (!fileManager->initialize()) {
        Serial.println("FATAL: File system initialization failed!");
    }
    
    // Reset file positions
    fileManager->setCurrentPos(0);
    
    // Reinitialize DAC + Laser
    initDAC();
    initTTL();

    // Reinitialize projector
    projector->init(fileManager, systemState);

    Serial.println("Reset complete");
    Serial.println("Status:");
    systemState->printStatus();
    Serial.println("--------------------------------");
}

uint32_t CommandProcessor::calculateDelayUs(uint8_t payload[RECORD_SIZE]) {
    uint8_t freq_KHz_lower = payload[0];
    uint8_t freq_KHz_upper = payload[1];
    uint16_t freq_KHz = (freq_KHz_upper << 8) | freq_KHz_lower;
    
    if (freq_KHz >= 854) {
        Serial.println("PLAY: (full speed)");
        return 0;  // No delay for full speed
    } else if (freq_KHz == 0) {
        Serial.println("DEBUG SPEED: 10Hz");
        return 100000; 
    } else {
        Serial.print("PLAY: ");
        Serial.print(freq_KHz);
        Serial.println("KHz");
        return 1000 / freq_KHz;
    }
}

void CommandProcessor::printCommandInfo(uint8_t cmd, const char* description) {
    Serial.print("Command: ");
    Serial.print(description);
    Serial.print(" (0b");
    Serial.print(cmd, BIN);
    Serial.println(")");
}
