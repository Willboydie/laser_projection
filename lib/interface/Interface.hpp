#pragma once

#include <Arduino.h>
#include "../file_manager/FileManager.hpp"
#include "../system_state/SystemState.hpp"
#include "../projector/Projector.hpp"
#include "../utils/constants.h"
#include "../system/System.hpp"
#include "../debug/Debug.hpp"

// Incoming commands
#define SLEEP_CMD  0b11100000
#define WAKE_CMD  0b11100001
#define BOOTLOADER_CMD  0b11100010
#define ENABLE_DISABLE_CMD  0b10001111
#define RESTART_CMD  0b10001110
#define WRITE_MODE_CMD  0b00000010
#define PROJECT_MODE_CMD  0b00000011
#define UPLOAD_CMD    0b00000001
#define PLAY_PAUSE_CMD 0b00001001
#define SET_PLAYBACK_SPEED_CMD 0b00001010
#define SET_LASER_DUTY_CYCLE_CMD 0b00011011
#define SEND_STATUS_CMD 0b00001011
#define DEBUG_MODE_CMD 0b00011010

#define TOGGLE_PROJECT_MODE_CMD 0b00111010
#define SET_POINT_CMD 0b00111011

// outgoing
#define RECEIVE_STATUS_PACKET_CMD 0b11111010

class Interface {  
public:
    Interface(FileManager& fm, SystemState& ss, Projector& proj, System& sys, Laser& laser);
    ~Interface() = default;
    void listen();
    
private:
    FileManager& fileManager;
    SystemState& systemState;
    Projector& projector;
    System& system;
    Laser& laser;

    void processCommand(uint8_t cmd, uint8_t* payload, uint32_t length);

    // Command handlers
    void handleSleepCommand();
    void handleWakeCommand();
    void handleBootloaderCommand();
    void handleEnableDisableCommand();
    void handleRestartCommand();
    void handleWriteModeCommand();
    void handleProjectModeCommand();
    void handleUploadCommand(uint8_t* payload, uint32_t length);
    void handlePlayPauseCommand();
    void handleSetPlaybackSpeedCommand(uint8_t* payload, uint32_t length);
    void handleSetLaserDutyCycleCommand(uint8_t* payload, uint32_t length);
    void handleSendStatusCommand();
    void handleDebugModeCommand();
    void handleToggleProjectModeCommand();
    void handleSetPointCommand(uint8_t* payload, uint32_t length);

    // Utility functions
    uint32_t calculateDelayUs(uint8_t* payload, uint32_t length);
    void printCommandInfo(uint8_t cmd, const char* description);
    uint16_t readLength();
    float calculateDutyCycle(uint8_t* payload, uint32_t length);

    uint32_t lastStatusPacketTime = 1;
};