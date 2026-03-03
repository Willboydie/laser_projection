#include "RotatingBuffer.hpp"

constexpr uint32_t buffer_size = BUFFER_SIZE;
constexpr uint32_t record_size = RECORD_SIZE;

RotatingBuffer::RotatingBuffer(FileManager& _fileManager)
: fileManager(_fileManager)
{}

bool RotatingBuffer::init() {
    if (!fileManager.isInitialised()) {
        Serial.println("ERROR: File manager is not initialised!");
        return false;
    }

    // Update file size and open for reading
    if (fileManager.openForRead()) {
        fileManager.setCurrentPos(0);
        fileManager.seekToPosition(0);
        filesize = fileManager.getFileSize();

        if (filesize == 0) {
            Serial.println("ERROR: File size is 0");
            return false;
        }
        Serial.print("SD file opened for read: ");
        Serial.print(filesize);
        Serial.println(" bytes");
    } else {
        Serial.println("ERROR: Failed to prepare for buffer fill. Buffers not initialised.");
        return false;
    }
    
    Serial.println("Initializing rotating buffer system...");
    fillingBuffer = bufferA;
    if (!fill()) {
        Serial.println("ERROR: Failed to fill buffer A");
        return false;
    }
    Serial.println("Buffer A filled");
    
    initialised = true;
    Serial.println("Rotating buffer system initialised successfully");
    return true;
}

void RotatingBuffer::deinit() {
    Serial.println("Deinitialising rotating buffer system...");
    bufferIdx = 0;
    clearBuffers();
    initialised = false;
    Serial.println("Rotating buffer system deinitialised successfully. User must re-initialise.");
}


void RotatingBuffer::updateValues(uint16_t* valA, uint16_t* valB, uint8_t* laserState) {
    if (!initialised) {
        Serial.println("ERROR: Buffer system not initialised");
        return;
    }
    
    uint32_t offset = (bufferIdx << 2) + bufferIdx; // *5 without multiplication
    *valA = (activeBuffer[offset] << 8) | activeBuffer[offset + 1];
    *valB = (activeBuffer[offset + 2] << 8) | activeBuffer[offset + 3];
    *laserState = activeBuffer[offset + 4];
    bufferIdx++;
}

bool RotatingBuffer::fill() {
    if (!initialised) {
        Serial.println("ERROR: Rotating buffer system not initialised");
        return false;
    }
    size_t bytesRead;
    if (filesize - fileManager.getCurrentPos() > buffer_size * record_size) {
        bytesRead = read(fillingBuffer, buffer_size * record_size);
    } else {
        bytesRead = read(fillingBuffer, filesize - fileManager.getCurrentPos());
        bytesRead = read(fillingBuffer + bytesRead, buffer_size * record_size - bytesRead);
    }    
    return true;
}

int RotatingBuffer::read(uint8_t* buffer, size_t bytesToRead) {

    // SdFat automatically uses DMA for large reads
    FsFile& file = fileManager.getPlaybackFile();
    if (!file) {
        Serial.println("ERROR: Failed to open file");
        return 0;
    }

    size_t bytesRead = file.read(buffer, bytesToRead);
    if (bytesRead == bytesToRead) {
        fileManager.setCurrentPos((fileManager.getCurrentPos() + bytesRead) % filesize);
        fileManager.seekToPosition(fileManager.getCurrentPos());
        return bytesRead;

    } else {
        Serial.println("ERROR: Failed to read buffer data");
        return 0;
    }
}

bool RotatingBuffer::rotate() {
    oldActive = activeBuffer;
    oldReady = readyBuffer;
    oldFill = fillingBuffer;
    
    activeBuffer = oldReady;
    readyBuffer = oldFill;
    fillingBuffer = oldActive;

    bufferIdx = 0;

    if (!fill()) {
        Serial.println("ERROR: Failed to fill buffer");
        return false;
    }
    return true;
}

void RotatingBuffer::clearBuffers() {
    memset(bufferA, 0, BUFFER_SIZE * RECORD_SIZE);
    memset(bufferB, 0, BUFFER_SIZE * RECORD_SIZE);
    memset(bufferC, 0, BUFFER_SIZE * RECORD_SIZE);
}