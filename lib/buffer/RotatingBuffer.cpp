#include "RotatingBuffer.hpp"

constexpr uint32_t buffer_size = BUFFER_SIZE;
constexpr uint32_t record_size = RECORD_SIZE;

RotatingBuffer::RotatingBuffer(FileManager& _fileManager)
: fileManager(_fileManager)
{}

bool RotatingBuffer::init() {
    if (!fileManager.isInitialised()) {
        Debug::msg("ERROR: File manager is not initialised!");
        return false;
    }

    if (fileManager.openForRead()) {
        fileManager.setCurrentPos(0);
        fileManager.seekToPosition(0);
        filesize = fileManager.getFileSize();

        if (filesize == 0) {
            Debug::msg("ERROR: File size is 0");
            return false;
        }
        Debug::msg("SD file opened for read: ");
        Debug::msg(filesize);
        Debug::msg(" bytes");
    } else {
        Debug::msg("ERROR: Failed to prepare for buffer fill. Buffers not initialised.");
        return false;
    }

    initialised = true;
    Debug::msg("Initializing rotating buffer system...");

    fillingBuffer = bufferA;
    if (!fill()) {
        Debug::msg("ERROR: Failed to fill buffer A");
        return false;
    }
    fillingBuffer = bufferB;
    if (!fill()) {
        Debug::msg("ERROR: Failed to fill buffer B");
        return false;
    }
    
    activeBuffer  = bufferA;
    readyBuffer   = bufferB;
    fillingBuffer = bufferC;

    if (!fill()) {
        Debug::msg("ERROR: Failed to fill buffer C");
        return false;
    }
    Debug::msg("Rotating buffer system initialised successfully");
    return true;
}

void RotatingBuffer::deinit() {
    Debug::msg("Deinitialising rotating buffer system...");
    bufferIdx = 0;
    needsFill = false;
    clearBuffers();
    initialised = false;
    Debug::msg("Rotating buffer system deinitialised successfully. User must re-initialise.");
}


void RotatingBuffer::updateValues(uint16_t& valA, uint16_t& valB, bool& laserState) {
    if (!initialised) {
        Debug::msg("ERROR: Buffer system not initialised");
        return;
    }
    
    uint32_t offset = (bufferIdx << 2) + bufferIdx;
    valA = (activeBuffer[offset] << 8) | activeBuffer[offset + 1];
    valB = (activeBuffer[offset + 2] << 8) | activeBuffer[offset + 3];
    laserState = activeBuffer[offset + 4];
    bufferIdx++;
}

bool RotatingBuffer::fill() {
    if (!initialised) {
        Debug::msg("ERROR: Rotating buffer system not initialised");
        return false;
    }

    FsFile& file = fileManager.getPlaybackFile();
    if (!file) {
        Debug::msg("ERROR: Failed to open file");
        return false;
    }

    uint8_t* dest = fillingBuffer;

    const size_t totalBytes = buffer_size * record_size;
    size_t remaining = filesize - fileManager.getCurrentPos();

    if (remaining >= totalBytes) {
        size_t n = file.read(dest, totalBytes);
        if (n != totalBytes) {
            Debug::msg("ERROR: Failed to read buffer data");
            return false;
        }
        fileManager.setCurrentPos(fileManager.getCurrentPos() + totalBytes);
    } else {
        size_t n = file.read(dest, remaining);
        if (n != remaining) {
            Debug::msg("ERROR: Failed to read tail");
            return false;
        }

        file.seek(0);

        size_t head = totalBytes - remaining;
        n = file.read(dest + remaining, head);
        if (n != head) {
            Debug::msg("ERROR: Failed to read head");
            return false;
        }
        fileManager.setCurrentPos(head);
    }

    return true;
}

bool RotatingBuffer::fillIfNeeded() {
    if (!needsFill) return true;
    if (!fill()) return false;
    needsFill = false;
    return true;
}

bool RotatingBuffer::rotate() {
    if (needsFill) {
        bufferIdx = 0;
        return false;
    }

    uint8_t* oldActive = activeBuffer;
    activeBuffer = readyBuffer;
    readyBuffer = fillingBuffer;
    fillingBuffer = oldActive;

    bufferIdx = 0;
    needsFill = true;
    return true;
}

void RotatingBuffer::clearBuffers() {
    memset(bufferA, 0, BUFFER_SIZE * RECORD_SIZE);
    memset(bufferB, 0, BUFFER_SIZE * RECORD_SIZE);
    memset(bufferC, 0, BUFFER_SIZE * RECORD_SIZE);
}
