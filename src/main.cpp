#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <DAC.h>
#include <TTL.h>

// ======== CONFIGURATION ========
#define WRITE_CMD    0b0001
#define PROJECT_CMD  0b0010
#define SAVE_CMD     0b0100
#define PLAY_CMD     0b1001
#define PAUSE_CMD    0b1010
#define RESET_CMD    0b1111

#define SD_CS_PIN BUILTIN_SDCARD    // SD card chip select

// ======== GLOBALS ========
enum Mode { MODE_WRITE, MODE_PROJECT };
Mode currentMode = MODE_WRITE;
bool isProjecting = false;
File dataFile;

// Track file positions
uint32_t bytesWritten = 0;  // Total bytes written in WRITE mode
uint32_t bytesRead = 0;     // Current read position in PROJECT mode
uint32_t fileSize = 0;      // Total file size (bytes)

File playbackFile;          // File handle for playback
uint32_t currentDelayUs = 0; // Default delay for playback (none)

// ======== SETUP HELPER FUNCTIONS ========
void updateFileSize() {
  dataFile = SD.open("data.bin", FILE_READ);
  if (dataFile) {
    fileSize = dataFile.size();
    dataFile.close();
  } else {
    Serial.println("SD ERROR: Failed to open data.bin!");
    fileSize = 0;
  }
}

// ======== SETUP ========
void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Initialize SD card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD ERROR: Initialization failed!");
    while (1);
  }
  Serial.println("SD initialized.");

  // Open file and initialize sizes
  updateFileSize();
  Serial.print("Initial file size: "); Serial.print(fileSize); Serial.println(" bytes");

  initDAC();
  initTTL();
}

// ======== LOOP HELPER FUNCTIONS ========
uint32_t calculateDelayUs(uint32_t payload) {
  if (payload == 0) {
    Serial.println("PLAY: 250KHz (full speed).");
    return 0;
  }
  for (uint8_t bit = 0; bit < 32; bit++) {
    if (payload & (1UL << bit)) { // bit 0 = least significant
      uint32_t delay = 1;
      for (uint8_t i = 0; i < bit; i++) delay *= 10;
      float freq_KHz = 1000.0 / (delay + 4);
      Serial.print("PLAY: "); Serial.print(freq_KHz, 1); 
      Serial.print("KHz.\n");
      return delay;
    }
  }
  return 1;
}

void wipeFile() {
  SD.remove("data.bin");
  dataFile = SD.open("data.bin", FILE_WRITE);
  if (dataFile) {
    dataFile.close();
    bytesWritten = 0;
    fileSize = 0;
    Serial.println("SD: File wiped.");
  } else {
    Serial.println("SD ERROR: Failed to wipe file!");
  }
}

// ======== COMMAND PROCESSING ========
void processCommand(uint8_t cmd, uint64_t payload) {  // Changed to uint64_t

  if (cmd == WRITE_CMD || cmd == PROJECT_CMD) {
    if (playbackFile) {
      playbackFile.close(); // Close file on mode switch
    }
  }

  switch (cmd) {
    // --- Mode switches ---
    case RESET_CMD:
      Serial.println("Initiating Reset...");
      isProjecting = false;
      bytesRead = 0;
      initDAC();
      break;

    case WRITE_CMD:
      wipeFile();
      currentMode = MODE_WRITE;
      bytesRead = 0; // Reset read counter
      Serial.println("MODE: WRITE (ready to save)");
      break;

    case PROJECT_CMD:
      currentMode = MODE_PROJECT;
      isProjecting = false;
      updateFileSize(); // Refresh file size
      Serial.println("MODE: PROJECT (playback ready)");
      break;

    // --- Write mode commands ---
    case SAVE_CMD:
      // No longer used for per-point writes; raw 6-byte packets are handled in loop() when in MODE_WRITE
      break;

    // --- Project mode commands ---
    case PLAY_CMD:
      if (currentMode == MODE_PROJECT) {

        const uint32_t recordSize = sizeof(uint16_t) * 2 + sizeof(uint8_t);  // 5 bytes per record
        updateFileSize();
        if (fileSize < recordSize) {
          Serial.println("PLAYBACK ERROR: File too small or empty for a record.");
          isProjecting = false;
          return;
        }

        if (!playbackFile) {
          playbackFile = SD.open("data.bin", FILE_READ);
          if (!playbackFile) {
            Serial.println("SD ERROR: Failed to open data.bin!");
            isProjecting = false;
            return;
          }
          playbackFile.seek(bytesRead % fileSize); // Resume from last position
        }
        
        currentDelayUs = calculateDelayUs((uint32_t)(payload & 0xFFFFFFFF));  // FIXED: Cast to uint32_t
        isProjecting = true;
        Serial.print("PLAYBACK: Started from byte ");
        Serial.print(bytesRead % fileSize); // Resume position
        Serial.print("/"); Serial.println(fileSize);
      }
      break;

    case PAUSE_CMD:
      if (currentMode == MODE_PROJECT) {
        isProjecting = !isProjecting;
        Serial.print("PLAYBACK: ");
        Serial.println(isProjecting ? "Resumed" : "Paused");
        Serial.print("Current position: ");
        Serial.print(bytesRead % fileSize); Serial.print("/");
        Serial.println(fileSize);
      }
      break;

    default:
      Serial.print("ERROR: Unknown command 0b");
      Serial.println(cmd, BIN);
  }
}

int i = 0;
uint32_t loopStart;
uint32_t timeForSerialCheck;
uint32_t timeForBinaryRead;
uint32_t timeForDACUpdate;
uint32_t timeForIfStatement;
bool laser_on = true;

// ======== MAIN LOOP ========
void loop() {

  loopStart = micros();

  // --- Check for incoming 40-bit command ---
  if (Serial.available() => 5) {
    uint8_t b0 = Serial.read();       // first byte
    uint8_t cmd = (b0 >> 4) & 0x0F;  // high nibble = command
    uint64_t payload = b0 & 0x0F;    // low nibble = top 4 bits of payload

    for (int i = 1; i < 6; i++) {
        uint8_t b = Serial.read();
        payload = (payload << 8) | b;
    }
    // Serial.print("RX CMD: 0b"); Serial.print(cmd, BIN);
    // Serial.print(", Payload: 0b"); Serial.println((uint32_t)payload, BIN);
    processCommand(cmd, payload);
  }

  timeForSerialCheck = micros() - loopStart;

  // --- In WRITE mode, accept raw 6-byte packets (cmd, valA, valB, laser_on) ---
  if (currentMode == MODE_WRITE) {
    while (Serial.available() >= 6) {
      uint8_t cmd_byte;
      uint8_t rest[5];
      Serial.readBytes(&cmd_byte, 1);
      Serial.readBytes(rest, 5);
      // Save ONLY last 5 bytes to SD
      dataFile = SD.open("data.bin", FILE_WRITE);
      if (dataFile) {
        size_t w = dataFile.write(rest, 5);
        dataFile.flush();
        dataFile.close();
        if (w == 5) { bytesWritten += 5; fileSize = bytesWritten; }
      } else {
        Serial.println("SD ERROR: Failed to open file for write packet!");
      }
    }
  }

  // --- Projection loop ---
  if (currentMode == MODE_PROJECT && isProjecting && fileSize > 4) {  // FIXED: Changed from 2 to 4 (minimum record size)
    static uint32_t lastUpdateTime = 0;
    static uint16_t valA, valB;
    static uint8_t on_off_signal;

    if (micros() - lastUpdateTime >= currentDelayUs) {

      timeForIfStatement = micros() - loopStart - timeForSerialCheck;

      const int recordSize = sizeof(valA) + sizeof(valB) + sizeof(on_off_signal); // 5 bytes on disk
      if (playbackFile.available() >= recordSize) {
        playbackFile.read((byte*)&valA, sizeof(valA));
        playbackFile.read((byte*)&valB, sizeof(valB));
        playbackFile.read((byte*)&laser_on, sizeof(on_off_signal));
        bytesRead += recordSize;
        bytesRead %= fileSize; // Wrap around

        timeForBinaryRead = micros() - loopStart - timeForSerialCheck - timeForIfStatement;

        if (laser_on != on_off_signal) {
          if (laser_on) {
            toggleLaserOn();
          } else {
            toggleLaserOff();
          }
          laser_on = on_off_signal;
        }

        updateDAC(valA, valB);

        timeForDACUpdate = micros() - loopStart - timeForSerialCheck - timeForIfStatement - timeForBinaryRead;

      } else {
        playbackFile.seek(0); // Loop to start
        bytesRead = 0;
      }

      lastUpdateTime = micros();
    }
  }
  
  i++; 
  if (i % 1000000 == 0 && isProjecting && currentMode == MODE_PROJECT) {
    Serial.print("Loop time (µs): "); Serial.print(micros() - loopStart);
    Serial.print("µs | Serial check: "); Serial.print(timeForSerialCheck);
    Serial.print("µs | Binary read: "); Serial.print(timeForBinaryRead);
    Serial.print("µs | DAC update: "); Serial.print(timeForDACUpdate);
    Serial.println();
  }
}

// We are seeing a loop every 4 micro seconds (250kHz) --- ~4 microseconds are spent on updateDAC()
// We have ~6000 points in the file, so we loop ~0.025s (40 loops per second)


// Why we see distortion:
// The current XYZ -> (theta, phi) conversion assumes that r will stay constant, which is of course bullshit.