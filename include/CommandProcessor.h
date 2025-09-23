#ifndef COMMANDPROCESSOR_H
#define COMMANDPROCESSOR_H

#include <Arduino.h>
#include "FileManager.h"
#include "SystemState.h"
#include "Projector.h"

// Command definitions
#define WRITE_CMD    0b00000001
#define PROJECT_CMD  0b00000010
#define SAVE_CMD     0b00000100
#define PLAY_CMD     0b00001001
#define PAUSE_CMD    0b00001010
#define RESET_CMD    0b00001111

#define RECORD_SIZE 5

class CommandProcessor {
private:
    FileManager* fileManager;
    SystemState* systemState;
    Projector* projector;
    
public:
    CommandProcessor(FileManager* fm, SystemState* ss, Projector* proj);
    void processCommand(uint8_t cmd, uint8_t payload[RECORD_SIZE]);
    
private:
    // Command handlers
    void handleWriteCommand();
    void handleProjectCommand();
    void handleSaveCommand(uint8_t payload[RECORD_SIZE]);
    void handlePlayCommand(uint8_t payload[RECORD_SIZE]);
    void handlePauseCommand();
    void handleResetCommand();
    
    // Utility functions
    uint32_t calculateDelayUs(uint8_t payload[RECORD_SIZE]);
    void printCommandInfo(uint8_t cmd, const char* description);
};

#endif
