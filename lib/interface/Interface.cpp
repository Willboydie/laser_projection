#include "Interface.hpp"

Interface::Interface(FileManager& fm, SystemState& ss, Projector& proj, System& sys) 
    : fileManager(fm)
    , systemState(ss)
    , projector(proj) 
    , system(sys)
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

        case ENABLE_DISABLE_CMD:
            handleEnableDisableCommand();
            break;

        case RESTART_CMD:
            handleRestartCommand();
            break;        
            
        case WRITE_MODE_CMD:
            handleWriteModeCommand();
            break;

        case PROJECT_MODE_CMD:
            handleProjectModeCommand();
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
    system.sleep();
}

void Interface::handleWakeCommand() {
    Serial.println("Waking up...");
    system.wake();
}

void Interface::handleBootloaderCommand() {
    Serial.println("Entering bootloader mode...");
    system.activateBootloader();
}

void Interface::handleEnableDisableCommand() {
    if (systemState.enabled) {
        system.disable();
    } else {
        system.enable();
    }
}

void Interface::handleRestartCommand() {
    system.restart();
}


void Interface::handleWriteModeCommand() {
    system.enterWriteMode();
}

void Interface::handleProjectModeCommand() {
    system.enterProjectMode();
}


void Interface::handleUploadCommand(uint8_t* payload, uint16_t length) {
    if (!systemState.enabled) return;
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
    system.playPause();
}


void Interface::handleSetPlaybackSpeedCommand(uint8_t* payload, uint16_t length) {
    uint32_t delayUs = calculateDelayUs(payload, length);
    projector.setDelay(delayUs);
}


void Interface::handleSendStatusCommand() {
    Serial.println("Sending status...");
    // do things...
}



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