#include "Interface.hpp"

Interface::Interface(FileManager& fm, SystemState& ss, Projector& proj, System& sys, Laser& laser) 
    : fileManager(fm)
    , systemState(ss)
    , projector(proj) 
    , system(sys)
    , laser(laser)
    {}

void Interface::listen() {
    // Need at least cmd (1) + length (2) bytes; 0-length payload still needs 3 bytes
    if (Serial.available() >= 3) {
        uint8_t cmd = Serial.read();
        uint32_t length = readLength();
  
        uint8_t payload[length]; 
        Serial.readBytes((char*)payload, length);
  
        processCommand(cmd, payload, length);
      }
}

void Interface::processCommand(uint8_t cmd, uint8_t* payload, uint32_t length) {
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
        
        case SET_LASER_DUTY_CYCLE_CMD:
            handleSetLaserDutyCycleCommand(payload, length);
            break;
            
        case SEND_STATUS_CMD:
            handleSendStatusCommand();
            break;
            
        case DEBUG_MODE_CMD:
            handleDebugModeCommand();
            break;

        case TOGGLE_PROJECT_MODE_CMD:
            handleToggleProjectModeCommand();
            break;
        
        case SET_POINT_CMD:
            handleSetPointCommand(payload, length);
            break;

        default:
            Debug::msg("ERROR: Unknown command");
    }
}

void Interface::handleSleepCommand() {
    Debug::msg("Sleeping...");
    system.sleep();
}

void Interface::handleWakeCommand() {
    Debug::msg("Waking up...");
    system.wake();
}

void Interface::handleBootloaderCommand() {
    Debug::msg("Entering bootloader mode...");
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


void Interface::handleUploadCommand(uint8_t* payload, uint32_t length) {
    if (!systemState.enabled) return;
    if (systemState.mode != MODE_WRITE) {
        Debug::msg("ERROR: Not in WRITE mode");
        return;
    }
    
    if (!fileManager.writeRecords(payload, length)) {
       Debug::msg("ERROR: Failed to save record");
       return;
    }
}

void Interface::handlePlayPauseCommand() {
    system.playPause();
}


void Interface::handleSetPlaybackSpeedCommand(uint8_t* payload, uint32_t length) {
    uint32_t delayUs = calculateDelayUs(payload, length);
    projector.setDelay(delayUs);
}

void Interface::handleSetLaserDutyCycleCommand(uint8_t* payload, uint32_t length) {
    float dutyCycle = calculateDutyCycle(payload, length);
    laser.setDutyCycle(dutyCycle);
}


void Interface::handleDebugModeCommand() {
    Debug::mode = !Debug::mode;
}

void Interface::handleToggleProjectModeCommand() {
    system.toggleProjectMode();
}

void Interface::handleSetPointCommand(uint8_t* payload, uint32_t length) {
    if (length != 4) {
        Debug::msg("ERROR: Invalid payload length for set point. Expected 4 bytes, got " + String(length));
        return;
    }
    uint16_t x = (payload[0] | (payload[1] << 8)); // little endian
    uint16_t y = (payload[2] | (payload[3] << 8)); // little endian
    projector.setSetPointX(x);
    projector.setSetPointY(y);
    Debug::msg("Set point: (" + String(x) + ", " + String(y) + ")");
}


void Interface::handleSendStatusCommand() {
    uint32_t timeSinceLastStatusPacket = millis() - lastStatusPacketTime;
    if (timeSinceLastStatusPacket < STATUS_PACKET_INTERVAL) return;

    uint8_t power_state = systemState.powerState == POWER_AWAKE ? 1 : 0;
    uint8_t mode = systemState.mode == MODE_WRITE ? 2 : systemState.mode == MODE_PROJECT ? 1 : 0;
    uint8_t enabled = systemState.enabled ? 1 : 0;
    uint8_t projecting = systemState.projecting ? 1 : 0;
    uint8_t laser_duty_cycle = (uint8_t)(laser.getDutyCycle() * 100);
    uint8_t projector_delay = (uint8_t)(projector.getDelay()); // in microseconds
    uint16_t points_frequency_KHz;
    uint8_t points_freq_KHz_lower;
    uint8_t points_freq_KHz_upper;
    uint8_t debug_mode = Debug::mode ? 1 : 0;
    uint32_t file_size = fileManager.getFileSize();
    uint8_t project_mode = systemState.projectMode == PROJECT_MODE_PATH ? 0 : 1;

    if (mode == 1 && enabled && projecting) {
        points_frequency_KHz = projector.getPointsProjected() / timeSinceLastStatusPacket;
        projector.setPointsProjected(0);
    } else {
        points_frequency_KHz = 0;
    }
    points_freq_KHz_lower = (uint8_t)(points_frequency_KHz & 0xFF);
    points_freq_KHz_upper = (uint8_t)(points_frequency_KHz >> 8);

    uint16_t payload_length = 14;
    uint8_t header_length = 3;
    uint8_t statusPacket[payload_length + header_length];

    uint8_t payload_length_lower = (uint8_t)(payload_length & 0xFF); // little endian
    uint8_t payload_length_upper = (uint8_t)(payload_length >> 8); // little endian

    uint8_t file_size_lowest = (uint8_t)(file_size & 0xFF);
    uint8_t file_size_lower = (uint8_t)(file_size >> 8);
    uint8_t file_size_upper = (uint8_t)(file_size >> 16);
    uint8_t file_size_highest = (uint8_t)(file_size >> 24);
    
    statusPacket[0] = RECEIVE_STATUS_PACKET_CMD;
    statusPacket[1] = payload_length_lower;
    statusPacket[2] = payload_length_upper;
    statusPacket[3] = power_state;
    statusPacket[4] = mode;
    statusPacket[5] = enabled;
    statusPacket[6] = projecting;
    statusPacket[7] = laser_duty_cycle;
    statusPacket[8] = projector_delay;
    statusPacket[9] = points_freq_KHz_lower;
    statusPacket[10] = points_freq_KHz_upper;
    statusPacket[11] = file_size_lowest;
    statusPacket[12] = file_size_lower;
    statusPacket[13] = file_size_upper;
    statusPacket[14] = file_size_highest;
    statusPacket[15] = debug_mode;
    statusPacket[16] = project_mode;
    Serial.write(statusPacket, payload_length + header_length);
    Serial.flush();
    lastStatusPacketTime = millis();
}


// -------------- Utility functions -------------------
uint32_t Interface::calculateDelayUs(uint8_t* payload, uint32_t length) {
    if (length != 2) {
        Debug::msg("ERROR: Invalid payload length for playback speed. Expected 2 bytes, got " + String(length));
        return 0;
    }
    uint8_t freq_KHz_lower = payload[0];
    uint8_t freq_KHz_upper = payload[1];
    uint16_t freq_KHz = (freq_KHz_lower | (freq_KHz_upper << 8)); // little endian
    
    if (freq_KHz >= 200) {
        Debug::msg("PLAY: (full speed)");
        return 0;  // No delay for full speed
    } else if (freq_KHz > 0) {
        Debug::msg("PLAY: " + String(freq_KHz) + "KHz");
        return 1000 / freq_KHz;
    } else {
        Debug::msg("ERROR: Invalid playback speed");
        return 20;
    }
}

uint16_t Interface::readLength() {
    uint8_t length[2];
    Serial.readBytes((char*)length, 2);
    return static_cast<uint16_t>((length[0] | (length[1] << 8))); // little endian
}

float Interface::calculateDutyCycle(uint8_t* payload, uint32_t length) {
    if (length != 2) {
        Debug::msg("ERROR: Invalid payload length for laser duty cycle. Expected 2 bytes, got " + String(length));
        return 0.0f;
    }
    uint8_t dutyCycle_lower = payload[0];
    uint8_t dutyCycle_upper = payload[1];
    uint16_t dutyCycle = (dutyCycle_lower | (dutyCycle_upper << 8)); // little endian
    return static_cast<float>(dutyCycle) / 100.0f;
}