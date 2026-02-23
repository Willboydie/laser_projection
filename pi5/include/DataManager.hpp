#pragma once

#include "Constants.hpp"

class DataManager {

private:

    uint32_t pathLength = 0;
    bool binFileOpen = false;
    bool pathVecInitialized = false;

public:
    bool init();
    void reset();
    void erase();

    // getters and setters
    void setPathLength(uint32_t size) { pathLength = size };
    uint32_t getPathLength() const { return pathLength };
    bool isBinFileOpen() const { return binFileOpen };
    void setBinFileOpen(bool open) { binFileOpen = open };
    bool isPathVecInitialized() const { return pathVecInitialized };
    void setPathVecInitialized(bool initialized) { pathVecInitialized = initialized };

private:

    bool openBinFile();
    bool closeBinFile();
    bool writeToPathVec(uint8_t* buffer, uint32_t length);
    bool writeToBinFile(uint8_t* buffer, uint32_t length);
    bool readFromBinFile(uint8_t* buffer, uint32_t length);
    void wipeBinFile();
    void wipePathVec();
    void initializePathVec(uint32_t length);

}