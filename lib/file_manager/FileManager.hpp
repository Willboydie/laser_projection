#pragma once

#include <Arduino.h>
#include <SdFat.h>
#include <sdios.h>
#include "../utils/constants.h"
#include "../debug/Debug.hpp"

class FileManager {
public:
    FileManager() = default;
    ~FileManager() = default;

    bool init();
    void deinit();

    // Write operations
    bool openForWrite();
    bool writeRecords(const uint8_t* data, uint16_t length);
    
    // Read operations
    bool openForRead();
    bool seekToPosition(uint32_t position);
    
    // File management
    bool wipeFile();
    void closeAllFiles();

    bool isInitialised() const { return initialised; }
    bool playbackFileIsOpen() const { return playbackFileOpen; }
    bool writeFileIsOpen() const { return writeFileOpen; }
    
    uint32_t getFileSize() const { return fileSize; }
    uint32_t getBytesWritten() const { return bytesWritten; }
    

    bool setFileSize(size_t size) { fileSize = size; return true; }
    void setCurrentPos(uint32_t pos) { currentPos = pos; }
    FsFile& getPlaybackFile() { return playbackFile; }
    uint32_t getCurrentPos() const { return currentPos; }

private:
    SdFat sd; // (provides DMA support)
    
    // Use FsFile with SdFat
    FsFile playbackFile;
    FsFile writeFile;
    
    // File state
    uint32_t fileSize = 0;
    uint32_t currentPos = 0;
    uint32_t bytesWritten = 0;
    bool playbackFileOpen = false;
    bool writeFileOpen = false;
    
    // File positioning cache
    uint32_t lastSeekPosition = 0xFFFFFFFF;

    bool initialised = false;
    
    // Constants
    static constexpr const char* DATA_FILENAME = "data.bin";
};
