#ifndef COMMANDPROCESSOR_H
#define COMMANDPROCESSOR_H

#include <Arduino.h>
#include "FileManager.h"
#include "SystemState.h"
#include "Projector.h"
#include "Constants.h"

// Command definitions
#define SLEEP_CMD  0b11100000
#define WAKE_CMD  0b11100001
#define BOOTLOADER_CMD  0b11100010
#define START_STOP_CMD  0b10001111
#define RESTART_CMD  0b10001110
#define WRITE_PROJECT_CMD  0b00000010
#define UPLOAD_CMD    0b00000001
#define PLAY_PAUSE_CMD 0b00001001
#define SET_PLAYBACK_SPEED_CMD 0b00001010
#define SEND_STATUS_CMD 0b00001011


class CommandProcessor {  
public:
    CommandProcessor(FileManager* fm, SystemState* ss, Projector* proj);
    void listen();
    
private:
    FileManager* fileManager;
    SystemState* systemState;
    Projector* projector;

    void processCommand(uint8_t cmd, uint8_t* payload, uint16_t length);

    // Command handlers
    void handleSleepCommand();
    void handleWakeCommand();
    void handleBootloaderCommand();
    void handleStartStopCommand();
    void handleRestartCommand();
    void handleWriteProjectCommand();
    void handleUploadCommand(uint8_t* payload, uint16_t length);
    void handlePlayPauseCommand();
    void handleSetPlaybackSpeedCommand(uint8_t* payload, uint16_t length);
    void handleSendStatusCommand();
    
    // Utility functions
    uint32_t calculateDelayUs(uint8_t* payload, uint16_t length);
    void printCommandInfo(uint8_t cmd, const char* description);
    uint16_t readLength();
};

#endif
