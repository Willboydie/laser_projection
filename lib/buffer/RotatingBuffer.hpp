#pragma once

#include "../file_manager/FileManager.hpp"
#include "../utils/constants.h"
#include <Arduino.h>

class RotatingBuffer {
public:
    RotatingBuffer(FileManager& _fileManager);
    ~RotatingBuffer() = default;

    bool init();
    void deinit();
    void updateValues(uint16_t& valA, uint16_t& valB, bool& laserState);
    bool rotate();

private:
    FileManager& fileManager;
    
    bool fill();
    int read(uint8_t* buffer, size_t bytesToRead);
    void clearBuffers();

    // Three buffers aligned to 4 bytes (as required by SdFat)
    __attribute__((aligned(4))) 
    uint8_t bufferA[BUFFER_SIZE * RECORD_SIZE];
    
    __attribute__((aligned(4))) 
    uint8_t bufferB[BUFFER_SIZE * RECORD_SIZE];
    
    __attribute__((aligned(4))) 
    uint8_t bufferC[BUFFER_SIZE * RECORD_SIZE];
    
    uint8_t* activeBuffer = bufferA;
    uint8_t* fillingBuffer = bufferB;
    uint8_t* readyBuffer = bufferC;
    uint8_t* oldActive;
    uint8_t* oldReady;
    uint8_t* oldFill;

    uint32_t bufferIdx = 0;
    uint32_t filesize;
    bool initialised = false;
};