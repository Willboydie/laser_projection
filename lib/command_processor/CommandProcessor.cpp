#include "CommandProcessor.h"
#include "DAC.h"
#include "Projector.h"
#include "TTL.h"
#include "FileManager.h"
#include "SystemState.h"
#include "Constants.h"

CommandProcessor::CommandProcessor(FileManager* fm, SystemState* ss, Projector* proj) 
    : fileManager(fm)
    , systemState(ss)
    , projector(proj) 
    {}

void CommandProcessor::listen() {
    if (Serial.available() >= 2) { // 1 command + 5 payload bytes
        cmd = Serial.read();
        uint16_t length = readLength();
  
        uint8_t payload[length]; 
        Serial.readBytes((char*)payload, length);
  
        // Process command
        processCommand(cmd, payload, length);
      }
}

void CommandProcessor::processCommand(uint8_t cmd, uint8_t* payload, uint16_t length) {
    switch (cmd) {
        case SLEEP_CMD:
            printCommandInfo(cmd, "SLEEP");
            handleSleepCommand();
            break;

        case WAKE_CMD:
            printCommandInfo(cmd, "WAKE");
            handleWakeCommand();
            break;

        case BOOTLOADER_CMD:
            printCommandInfo(cmd, "BOOTLOADER");
            handleBootloaderCommand();
            break;

        case START_STOP_CMD:
            printCommandInfo(cmd, "START/STOP");
            handleStartStopCommand();
            break;

        case RESTART_CMD:
            printCommandInfo(cmd, "RESTART");
            handleRestartCommand();
            break;        
            
        case WRITE_PROJECT_CMD:
            printCommandInfo(cmd, "WRITE/PROJECT");
            handleWriteProjectCommand();
            break;

        case UPLOAD_CMD:
            printCommandInfo(cmd, "UPLOAD");
            handleUploadCommand(payload, length);
            break;
            
        case PLAY_PAUSE_CMD:
            printCommandInfo(cmd, "PLAY/PAUSE");
            handlePlayPauseCommand();
            break;
            
        case SET_PLAYBACK_SPEED_CMD:
            printCommandInfo(cmd, "SET PLAYBACK SPEED:" + String(payload));
            handleSetPlaybackSpeedCommand(payload, length);
            break;
            
        case SEND_STATUS_CMD:
            printCommandInfo(cmd, "SEND STATUS");
            handleSendStatusCommand();
            break;
            
        default:
            Serial.print("ERROR: Unknown command 0b");
            Serial.println(cmd, BIN);
    }
}

void CommandProcessor::handleSleepCommand() {
    Serial.println("Sleeping...");
    systemState->setSleeping(true);
}

void CommandProcessor::handleWakeCommand() {
    Serial.println("Waking up...");
    systemState->setSleeping(false);
}

void CommandProcessor::handleBootloaderCommand() {
    Serial.println("Bootloader...");
    systemState->setBootloader(true);
}

void CommandProcessor::handleStartStopCommand() {
    if (systemState->isCurrentlyProjecting()) {
        systemState->setProjecting(false);
        Serial.println("Projection stopped");
    } else {
        systemState->setProjecting(true);
        Serial.println("Projection started");
    }
}

void CommandProcessor::handleRestartCommand() {
    Serial.println("Restarting...");
    systemState->setRestart(true);
}


void CommandProcessor::handleWriteProjectCommand() {
    // Close any open files
    fileManager->closeAllFiles();

    if (systemState->getMode() == SystemState::MODE_WRITE) {
        handleWriteCommand();
    } else if (systemState->getMode() == SystemState::MODE_PROJECT) {
        handleProjectCommand();
    } else {
        Serial.println("ERROR: Invalid mode");
    }
}

void CommandProcessor::handleWriteCommand() {
    // Wipe file and open for writing
    if (fileManager->wipeFile() && fileManager->openForWrite()) {
        systemState->setMode(SystemState::MODE_WRITE);
        systemState->setProjecting(false);
        Serial.println("Ready to save records");
    } else {
        Serial.println("ERROR: Failed to prepare for writing");
    }
}

void CommandProcessor::handleProjectCommand() {
    // Check file size
    if (fileManager->getFileSize() < RECORD_SIZE) {
        Serial.println("ERROR: File too small for playback. Cannot enter project mode.");
        return;
        }
    
    // Update file size and open for reading
    if (fileManager->openForRead()) {
        systemState->setMode(SystemState::MODE_PROJECT);
        systemState->setProjecting(false);
        fileManager->seekToPosition(fileManager->getCurrentPos());
        
        if (!projector->initializeBuffers()) {
            Serial.println("ERROR: Failed to initialize buffers");
            return;
        }
        Serial.println("Ready for playback");  
    } else {
        Serial.println("ERROR: Failed to prepare for projection");
    }
}


void CommandProcessor::handleUploadCommand(uint8_t* payload, uint16_t length) {
    if (systemState->getMode() != SystemState::MODE_WRITE) {
        Serial.println("ERROR: Not in WRITE mode");
        return;
    }
    
    if (!fileManager->writeRecords(payload, length)) {
       Serial.println("ERROR: Failed to save record");
       return;
    }
}

void CommandProcessor::handlePlayPauseCommand() {
    if (systemState->getMode() != SystemState::MODE_PROJECT) {
        Serial.println("ERROR: Not in PROJECT mode");
        return;
    }
    
    if (systemState->isCurrentlyProjecting()) {
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


void CommandProcessor::handleSetPlaybackSpeedCommand(uint8_t* payload, uint16_t length) {
    // Calculate delay
    uint32_t delayUs = calculateDelayUs(payload, length);
    systemState->setDelay(delayUs);
    
    // Initialize buffers first, then start projection
    if (projector->startProjection(delayUs)) {
        systemState->setProjecting(true);
        Serial.print("HIGH-SPEED PLAYBACK: Started with ");
        Serial.print(delayUs);
        Serial.println("μs delay");
    } else {
        Serial.println("ERROR: Failed to start projection");
        }
    }


void CommandProcessor::handleSendStatusCommand() {
    Serial.println("Sending status...");
    systemState->printStatus();
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
    
    // Reset file position
    fileManager->setCurrentPos(0);
    
    // Reinitialize DAC + Laser
    initDAC();
    initTTL();

    // Reinitialize projector
    projector->reset();

    Serial.println("Reset complete");
    Serial.println("Status:");
    systemState->printStatus();
    Serial.println("--------------------------------");
}


// -------------- Utility functions -------------------
uint32_t CommandProcessor::calculateDelayUs(uint8_t* payload, uint16_t length) {
    if (length != 2) {
        Serial.println("ERROR: Invalid payload length for playback speed. Expected 2 bytes, got " + String(length));
        return 0;
    }
    uint8_t freq_KHz_lower = payload[0];
    uint8_t freq_KHz_upper = payload[1];
    uint16_t freq_KHz = (freq_KHz_upper << 8) | freq_KHz_lower;
    
    if (freq_KHz >= 200) {
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

uint16_t CommandProcessor::readLength() {
    uint8_t length[2];
    Serial.readBytes((char*)length, 2);
    return static_cast<uint16_t>((length[0] << 8) | length[1]);
}