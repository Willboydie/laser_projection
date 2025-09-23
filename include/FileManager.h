#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <Arduino.h>
#include <SD.h>

class FileManager {
private:
    // File handles
    File playbackFile;
    File writeFile;
    
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
    File& getPlaybackFile() { return playbackFile; }
    uint32_t getCurrentPos() const { return currentPos; }
    void setCurrentPos(uint32_t pos) { currentPos = pos; }
    
    // File management
    bool wipeFile();
    void closeAllFiles();
    bool isPlaybackFileOpen() const { return playbackFileOpen; }
    bool isWriteFileOpen() const { return writeFileOpen; }
    
    // DMA support
    bool prepareForDMARead(uint32_t startPosition, size_t bytesToRead);
    
    // Status
    void printStatus() const;
};

#endif
