#ifndef COMMANDPROCESSOR_H
#define COMMANDPROCESSOR_H

#include <Arduino.h>
#include "FileManager.h"
#include "SystemState.h"
#include "Projector.h"
#include "Constants.h"

// Command definitions
#define WRITE_CMD    0b00000001
#define PROJECT_CMD  0b00000010
#define SAVE_CMD     0b00000100
#define PLAY_CMD     0b00001001
#define PAUSE_CMD    0b00001010
#define RESET_CMD    0b00001111


class CommandProcessor {
private:
    FileManager* fileManager;
    SystemState* systemState;
    Projector* projector;
    
public:
    CommandProcessor(FileManager* fm, SystemState* ss, Projector* proj);
    void processCommand(uint8_t cmd, uint8_t* payload, uint16_t length);
    static uint16_t readLength();

private:
    // Command handlers
    void handleWriteCommand();
    void handleProjectCommand();
    void handleSaveCommand(uint8_t* payload, uint16_t length);
    void handlePlayCommand(uint8_t* payload, uint16_t length);
    void handlePauseCommand();
    void handleResetCommand();
    
    // Utility functions
    uint32_t calculateDelayUs(uint8_t* payload, uint16_t length);
    void printCommandInfo(uint8_t cmd, const char* description);
};

#endif
