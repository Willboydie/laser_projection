#include "../file_manager/FileManager.hpp"
#include "../utils/constants.h"
#include <Arduino.h>

class RotatingBuffer {
public:
    RotatingBuffer(FileManager& _fileManager);
    ~RotatingBuffer() = default;

    bool init();
    void updateValues(uint32_t* activePos, uint16_t* valA, uint16_t* valB, uint8_t* laserState);
    bool rotate();

private:
    FileManager& fileManager;
    
    bool fill();
    int read(uint8_t* buffer, size_t bytesToRead);

    // Three buffers aligned to 4 bytes (as required by SdFat)
    __attribute__((aligned(4))) 
    uint8_t bufferA[BUFFER_SIZE * RECORD_SIZE];
    
    __attribute__((aligned(4))) 
    uint8_t bufferB[BUFFER_SIZE * RECORD_SIZE];
    
    __attribute__((aligned(4))) 
    uint8_t bufferC[BUFFER_SIZE * RECORD_SIZE];
    
    // Buffer management
    uint8_t* activeBuffer = bufferA;    // Currently projecting from this buffer
    uint8_t* fillingBuffer = bufferB;      // Currently filling this buffer  
    uint8_t* readyBuffer = bufferC;     // This buffer is ready to become active
    uint8_t* oldActive;
    uint8_t* oldReady;
    uint8_t* oldFill;

    uint32_t filesize;
    bool initialized = false;  // All buffers filled on startup
};