#pragma once

#include <Arduino.h>
#include "../file_manager/FileManager.hpp"
#include "../system_state/SystemState.hpp"
#include "../projector/Projector.hpp"
#include "../utils/constants.h"
#include "../system/System.hpp"

// Command definitions
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
#define SEND_STATUS_CMD 0b00001011


class Interface {  
public:
    Interface(FileManager& fm, SystemState& ss, Projector& proj, System& sys);
    ~Interface() = default;
    void listen();
    
private:
    FileManager& fileManager;
    SystemState& systemState;
    Projector& projector;
    System& system;

    void processCommand(uint8_t cmd, uint8_t* payload, uint16_t length);

    // Command handlers
    void handleSleepCommand();
    void handleWakeCommand();
    void handleBootloaderCommand();
    void handleEnableDisableCommand();
    void handleRestartCommand();
    void handleWriteModeCommand();
    void handleProjectModeCommand();
    void handleUploadCommand(uint8_t* payload, uint16_t length);
    void handlePlayPauseCommand();
    void handleSetPlaybackSpeedCommand(uint8_t* payload, uint16_t length);
    void handleSendStatusCommand();
    
    // Utility functions
    uint32_t calculateDelayUs(uint8_t* payload, uint16_t length);
    void printCommandInfo(uint8_t cmd, const char* description);
    uint16_t readLength();
};