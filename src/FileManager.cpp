#include "FileManager.h"


bool FileManager::initialize() {
    // Initialize SD card with SdFat (includes DMA support)
    if (!sd.begin(SdioConfig(FIFO_SDIO))) {
        Serial.println("SD ERROR: Initialization failed!");
        return false;
    }
    Serial.println("SD initialized with SdFat (DMA enabled).");
    
    // Get initial file size
    updateFileSize();
    Serial.print("Initial file size: ");
    Serial.print(fileSize);
    Serial.println(" bytes");
    
    return true;
}

bool FileManager::updateFileSize() {
    FsFile tempFile = sd.open(DATA_FILENAME, O_RDONLY);
    if (tempFile) {
        fileSize = tempFile.size();
        tempFile.close();
        return true;
    } else {
        Serial.println("SD ERROR: Failed to open data.bin!");
        fileSize = 0;
        return false;
    }
}

bool FileManager::openForWrite() {
    if (writeFileOpen) {
        writeFile.close();
    }
    
    writeFile = sd.open(DATA_FILENAME, O_WRONLY | O_CREAT | O_TRUNC);
    if (writeFile) {
        writeFileOpen = true;
        bytesWritten = 0;
        return true;
    } else {
        Serial.println("SD ERROR: Failed to open for write!");
        return false;
    }
}

bool FileManager::openForRead() {
    if (playbackFileOpen) {
        playbackFile.close();
    }
    
    playbackFile = sd.open(DATA_FILENAME, O_RDONLY);
    if (playbackFile) {
        playbackFileOpen = true;
        return true;
    } else {
        Serial.println("SD ERROR: Failed to open for read!");
        return false;
    }
}

bool FileManager::seekToPosition(uint32_t position) {
    if (!playbackFileOpen) {
        Serial.println("SD ERROR: Playback file not open!");
        return false;
    }
    
    // Only seek if position changed (optimization)
    if (position != lastSeekPosition) {
        playbackFile.seek(position % fileSize);
        lastSeekPosition = position;
    }
    
    return true;
}


bool FileManager::writeRecord(const uint8_t* data) {
    if (!writeFileOpen) {
        Serial.println("SD ERROR: Write file not open!");
        return false;
    }
    
    size_t written = writeFile.write(data, RECORD_SIZE);
    if (written == RECORD_SIZE) {
        bytesWritten += written;
        return true;
    } else {
        Serial.println("SD ERROR: Failed to write record!");
        return false;
    }
}

bool FileManager::wipeFile() {
    closeAllFiles();
    
    if (sd.remove(DATA_FILENAME)) {
        fileSize = 0;
        currentPos = 0;
        bytesWritten = 0;
        lastSeekPosition = 0xFFFFFFFF;
        Serial.println("SD: File wiped.");
        return true;
    } else {
        Serial.println("SD ERROR: Failed to wipe file!");
        return false;
    }
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

void FileManager::printStatus() const {
    Serial.print("File Status - Size: ");
    Serial.print(fileSize);
    Serial.print(", Read: ");
    Serial.print(currentPos);
    Serial.print(", Written: ");
    Serial.println(bytesWritten);
}
