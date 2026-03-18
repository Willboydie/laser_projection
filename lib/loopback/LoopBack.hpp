#pragma once

#include <Arduino.h>
#include <SPI.h>
#include "../file_manager/FileManager.hpp"
#include "../utils/constants.h"
#include "../debug/Debug.hpp"

// Physical loopback wiring (SPI0 master → SPI1 slave):
//   MOSI (pin 11) → pin 1  (SPI1 SDI)
//   SCK  (pin 13) → pin 27 (SPI1 SCK)
//   SYNC (pin 10) → pin 0  (SPI1 PCS0)

#define LOOPBACK_DATA_CMD 0b11111011

class LoopBack {
public:
    LoopBack(FileManager& fileManager);
    ~LoopBack() = default;

    bool init();
    void readNSave();
    bool isComplete() const { return complete; }

    uint32_t getPointsCaptured() const { return pointsCaptured; }
    void toggle();

    bool isOn() const { return on; }

private:
    FileManager& fileManager;

    void initSPISlave();
    void flushBuffer();
    void sendSerial();

    __attribute__((aligned(4)))
    uint8_t writeBuffer[LOOPBACK_BUFFER_SIZE * LOOPBACK_RECORD_SIZE];
    uint32_t writeIdx = 0;

    uint8_t rxFrame[6];
    uint8_t rxByteIdx = 0;

    uint32_t pointsCaptured = 0;
    uint32_t expectedPoints = 0;

    bool complete = false;
    bool initialised = false;

    uint32_t totalRxBytes = 0;
    uint32_t callCount = 0;
    uint32_t lastDiagTime = 0;

    bool on = false;
};
