#include "Interface.hpp"

Interface::Interface(FileManager& fm, SystemState& ss, Projector& proj) 
    : fileManager(fm)
    , systemState(ss)
    , projector(proj) 
    {}

void Interface::listen() {
    if (Serial.available() >= 2) { // 1 command + 5 payload bytes
        uint8_t cmd = Serial.read();
        uint16_t length = readLength();
  
        uint8_t payload[length]; 
        Serial.readBytes((char*)payload, length);
  
        processCommand(cmd, payload, length);
      }
}

void Interface::processCommand(uint8_t cmd, uint8_t* payload, uint16_t length) {
    switch (cmd) {
        case SLEEP_CMD:
            handleSleepCommand();
            break;

        case WAKE_CMD:
            handleWakeCommand();
            break;

        case BOOTLOADER_CMD:
            handleBootloaderCommand();
            break;

        case START_STOP_CMD:
            handleStartStopCommand();
            break;

        case RESTART_CMD:
            handleRestartCommand();
            break;        
            
        case WRITE_PROJECT_CMD:
            handleWriteProjectCommand();
            break;

        case UPLOAD_CMD:
            handleUploadCommand(payload, length);
            break;
            
        case PLAY_PAUSE_CMD:
            handlePlayPauseCommand();
            break;
            
        case SET_PLAYBACK_SPEED_CMD:
            handleSetPlaybackSpeedCommand(payload, length);
            break;
            
        case SEND_STATUS_CMD:
            handleSendStatusCommand();
            break;
            
        default:
            Serial.print("ERROR: Unknown command 0b");
            Serial.println(cmd, BIN);
    }
}

void Interface::handleSleepCommand() {
    Serial.println("Sleeping...");
    
}

void Interface::handleWakeCommand() {
    Serial.println("Waking up...");
    
}

void Interface::handleBootloaderCommand() {
    Serial.println("Bootloader...");
    
}

void Interface::handleStartStopCommand() {
// do things...
}

void Interface::handleRestartCommand() {
    Serial.println("Restarting...");
// do things...
}


void Interface::handleWriteProjectCommand() {
    // Close any open files
    fileManager.closeAllFiles();
    // do things ...
}

void Interface::handleWriteCommand() {
    // Wipe file and open for writing
    if (fileManager.wipeFile() && fileManager.openForWrite()) {
        systemState.mode = MODE_WRITE;
        systemState.projecting = false;
        Serial.println("Ready to save records");
    }
}

void Interface::handleProjectCommand() {
    // Check file size
    if (fileManager.getFileSize() < RECORD_SIZE) {
        Serial.println("ERROR: File too small for playback. Cannot enter project mode.");
        return;
        }
    
    // Update file size and open for reading
    if (fileManager.openForRead()) {
        systemState.mode = MODE_PROJECT;
        systemState.projecting = false;
        fileManager.seekToPosition(fileManager.getCurrentPos());
        
        if (!projector.initializeBuffers()) {
            Serial.println("ERROR: Failed to initialize buffers");
            return;
        }
        Serial.println("Ready for playback");  
    } else {
        Serial.println("ERROR: Failed to prepare for projection");
    }
}


void Interface::handleUploadCommand(uint8_t* payload, uint16_t length) {
    if (systemState.mode != MODE_WRITE) {
        Serial.println("ERROR: Not in WRITE mode");
        return;
    }
    
    if (!fileManager.writeRecords(payload, length)) {
       Serial.println("ERROR: Failed to save record");
       return;
    }
}

void Interface::handlePlayPauseCommand() {
    if (systemState.mode != MODE_PROJECT) {
        Serial.println("ERROR: Not in PROJECT mode");
        return;
    }
    
    if (systemState.projecting) {
        systemState.projecting = false;
        Serial.println("Projection paused");
    } else {
        systemState.projecting = true;
        Serial.println("Projection started");
    }
}


void Interface::handleSetPlaybackSpeedCommand(uint8_t* payload, uint16_t length) {
    // Calculate delay
    uint32_t delayUs = calculateDelayUs(payload, length);
    systemState->setDelay(delayUs);
    
    // do things..
}


void Interface::handleSendStatusCommand() {
    Serial.println("Sending status...");
    systemState->printStatus();
}


// void Interface::handleResetCommand() {
//     Serial.println("Initiating Reset...");
    
//     // Stop projection
//     projector->stopProjection();
//     systemState->setProjecting(false);
    
//     // Close all files
//     fileManager->closeAllFiles();

//     // Initialize file system
//     if (!fileManager->initialize()) {
//         Serial.println("FATAL: File system initialization failed!");
//     }
    
//     // Reset file position
//     fileManager->setCurrentPos(0);
    
//     // Reinitialize DAC + Laser
//     initDAC();
//     initTTL();

//     // Reinitialize projector
//     projector->reset();

//     Serial.println("Reset complete");
//     Serial.println("Status:");
//     systemState->printStatus();
//     Serial.println("--------------------------------");
// }


// -------------- Utility functions -------------------
uint32_t Interface::calculateDelayUs(uint8_t* payload, uint16_t length) {
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

uint16_t Interface::readLength() {
    uint8_t length[2];
    Serial.readBytes((char*)length, 2);
    return static_cast<uint16_t>((length[0] << 8) | length[1]);
}