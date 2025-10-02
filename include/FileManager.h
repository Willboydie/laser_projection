#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <Arduino.h>
#include <SdFat.h>
#include <sdios.h>
#include "Constants.h"

class FileManager {
private:
    // SdFat instance (provides DMA support)
    SdFat sd;
    
    // File handles - use FsFile with SdFat
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
    
    // Constants
    static constexpr const char* DATA_FILENAME = "data.bin";
    
public:
    // Initialization
    bool initialize();
    
    // File size management
    uint32_t getFileSize() const { return fileSize; }
    bool updateFileSize();
    
    // Write operations
    bool openForWrite();
    bool writeRecord(const uint8_t* data);
    uint32_t getBytesWritten() const { return bytesWritten; }
    
    // Read operations
    bool openForRead();
    bool seekToPosition(uint32_t position);
    FsFile& getPlaybackFile() { return playbackFile; }
    uint32_t getCurrentPos() const { return currentPos; }
    void setCurrentPos(uint32_t pos) { currentPos = pos; }
    
    // File management
    bool wipeFile();
    void closeAllFiles();
    bool isPlaybackFileOpen() const { return playbackFileOpen; }
    bool isWriteFileOpen() const { return writeFileOpen; }
    
    
    // Status
    void printStatus() const;
};

#endif
