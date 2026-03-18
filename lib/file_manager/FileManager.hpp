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
    bool writeRecords(const uint8_t* data, uint32_t length);
    
    // Read operations
    bool openForRead();
    bool seekToPosition(uint32_t position);
    
    // File management
    bool wipeFile();
    void closeAllFiles();

    // Loopback file operations
    bool openLoopbackForWrite();
    bool writeLoopback(const uint8_t* data, uint32_t length);
    void flushLoopback();
    uint32_t loopbackSize();
    bool openLoopbackForRead();
    int readLoopback(uint8_t* buffer, size_t length);
    void closeLoopback();
    void wipeLoopback();
    bool loopbackFileIsOpen() const { return loopbackOpen; }

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
    SdFat sd;
    
    FsFile playbackFile;
    FsFile writeFile;
    FsFile loopbackFile;
    
    uint32_t fileSize = 0;
    uint32_t currentPos = 0;
    uint32_t bytesWritten = 0;
    bool playbackFileOpen = false;
    bool writeFileOpen = false;
    bool loopbackOpen = false;

    uint32_t lastSeekPosition = 0xFFFFFFFF;

    bool initialised = false;
    
    static constexpr const char* DATA_FILENAME = "data.bin";
    static constexpr const char* LOOPBACK_FILENAME = "loopback.bin";
};
