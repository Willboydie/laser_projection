#include "FileManager.hpp"

bool FileManager::init() {
    if (!sd.begin(SdioConfig(FIFO_SDIO))) {
        Debug::msg("SD begin failed.");
        // Serial.print(" errCode=");
        // Serial.println(sd.card()->errorCode(), HEX);
        // Serial.print(" errData=");
        // Serial.println(sd.card()->errorData(), HEX);
        return false;
    }
    Debug::msg("SD initialized with SdFat (DMA enabled).");
    
    // Create file if it doesn't exist
    FsFile testFile = sd.open(DATA_FILENAME, O_RDONLY);
    if (!testFile) {
        // File doesn't exist, create it
        testFile = sd.open(DATA_FILENAME, O_WRONLY | O_CREAT);
        if (testFile) {
            testFile.close();
            Debug::msg("Created new data.bin file");
        } else {
            Debug::msg("SD ERROR: Failed to create data.bin!");
            return false;
        }
    } else {
        setFileSize(testFile.size());
        testFile.close();
    }
    
    initialised = true;
    Debug::msg("File size: ");
    Debug::msg(fileSize);
    return true;
}

void FileManager::deinit() {
    Debug::msg("Deinitializing file manager...");
    closeAllFiles();
    initialised = false;
    Debug::msg("File manager deinitialized successfully");
}
 
bool FileManager::openForWrite() {
    closeAllFiles();
    writeFile = sd.open(DATA_FILENAME, O_WRONLY | O_CREAT | O_TRUNC);
    if (!writeFile) {
        Debug::msg("SD ERROR: Failed to open for write!");
        return false;
    }
    writeFileOpen = true;
    bytesWritten = 0;
    Debug::msg("File opened for write");
    return true;
}

bool FileManager::openForRead() {
    closeAllFiles();
    playbackFile = sd.open(DATA_FILENAME, O_RDONLY);
    if (!playbackFile) {
        Debug::msg("SD ERROR: Failed to open for read!");
        return false;
    }
    playbackFileOpen = true;
    Debug::msg("File opened for read");
    Debug::msg("File size: " + String(fileSize));
    return true;
}

bool FileManager::seekToPosition(uint32_t position) {
    if (!playbackFileOpen) {
        Debug::msg("SD ERROR: Playback file not open!");
        return false;
    }
    if (fileSize == 0) {
        Debug::msg("SD ERROR: Cannot seek in empty file!");
        return false;
    }
    
    // Only seek if position changed (optimization)
    if (position != lastSeekPosition) {
        playbackFile.seek(position % fileSize);
        lastSeekPosition = position;
        currentPos = position;
    }
    return true;
}

bool FileManager::writeRecords(const uint8_t* data, uint16_t length) {
    if (!writeFileOpen) {
        Debug::msg("SD ERROR: Write file not open!");
        return false;
    }
    
    if (writeFile.write(data, length) != length) {
        Debug::msg("SD ERROR: Failed to write record!");
        return false;
    }
    bytesWritten += length;
    fileSize += length;
    return true;
}

bool FileManager::wipeFile() {
    closeAllFiles();
    if (!sd.remove(DATA_FILENAME)) {
        Debug::msg("SD ERROR: Failed to wipe file!");
        return false;
    }
    fileSize = currentPos = bytesWritten = 0;
    lastSeekPosition = 0xFFFFFFFF;
    Debug::msg("File wiped");
    return true;
}

void FileManager::closeAllFiles() {
    if (playbackFileOpen) {
        playbackFile.close();
        playbackFileOpen = false;
    }
    if (writeFileOpen) {
        writeFile.close();
        writeFileOpen = false;
    }
}