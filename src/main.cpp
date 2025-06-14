#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <DAC.h>

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
}

// ======== LOOP HELPER FUNCTIONS ========
uint32_t calculateDelayUs(uint32_t payload) {
  if (payload == 0) {
    Serial.println("PLAY: 250KHz (full speed).");
    return 0;
  }
  for (uint8_t bit = 0; bit < 32; bit++) {
    if (payload & (1 << (32 - bit))) { // LSB = bit 31
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

void verifyFileContents() {
    File verifyFile = SD.open("data.bin", FILE_READ);
    if (verifyFile) {
        Serial.println("File contents:");
        while (verifyFile.available() >= 4) {
            uint16_t a, b;
            verifyFile.read((byte*)&a, sizeof(a));
            verifyFile.read((byte*)&b, sizeof(b));
            Serial.print("A=0x"); Serial.print(a, HEX);
            Serial.print(", B=0x"); Serial.println(b, HEX);
        }
        verifyFile.close();
    } else {
        Serial.println("Could not open file for verification");
    }
}

// ======== COMMAND PROCESSING ========
void processCommand(uint8_t cmd, uint32_t payload) {

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

    case WRITE_CMD:
      wipeFile();
      currentMode = MODE_WRITE;
      bytesRead = 0; // Reset read counter
      Serial.println("MODE: WRITE (ready to save)");
      break;

    case PROJECT_CMD:
      // if (currentMode == MODE_WRITE) {
      //   verifyFileContents(); // Verify contents before switching to PROJECT
      // }
      currentMode = MODE_PROJECT;
      isProjecting = false;
      updateFileSize(); // Refresh file size
      Serial.println("MODE: PROJECT (playback ready)");
      break;

    // --- Write mode commands ---
    case SAVE_CMD:
      if (currentMode == MODE_WRITE) {
          uint16_t valA = (payload >> 16) & 0xFFFF;
          uint16_t valB = payload & 0xFFFF;
          dataFile = SD.open("data.bin", FILE_WRITE);
          if (dataFile) {
              size_t writtenA = dataFile.write((byte*)&valA, sizeof(valA));
              size_t writtenB = dataFile.write((byte*)&valB, sizeof(valB));
              dataFile.flush();  // Force write to disk
              dataFile.close();
              
              if (writtenA == sizeof(valA) && writtenB == sizeof(valB)) {
                  bytesWritten += sizeof(valA) + sizeof(valB);
                  fileSize = bytesWritten;
                  // Serial.print("SAVED: A=0b"); Serial.print(valA, BIN);
                  // Serial.print(", B=0b"); Serial.print(valB, BIN);
                  // Serial.print(" | Total bytes: "); Serial.println(fileSize);
              } else {
                  Serial.println("SD ERROR: Write failed!");
              }
          } else {
              Serial.println("SD ERROR: Failed to open file!");
          }
      }
      break;

    // --- Project mode commands ---
    case PLAY_CMD:
      if (currentMode == MODE_PROJECT) {

        if (!playbackFile) {
          playbackFile = SD.open("data.bin", FILE_READ);
          if (!playbackFile) {
            Serial.println("SD ERROR: Failed to open data.bin!");
            isProjecting = false;
            return;
          }
        playbackFile.seek(bytesRead % fileSize); // Resume from last position
        }
        
        currentDelayUs = calculateDelayUs(payload);
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

// int i = 0;
// int loopStart;
// int timeForSerialCheck;
// int timeForBinaryRead;
// int timeForDACUpdate;
// int timeForIfStatement;


// ======== MAIN LOOP ========
void loop() {

  // loopStart = micros();

  // --- Check for incoming 36-bit command ---
  if (Serial.available() > 4) {
    uint8_t cmd = Serial.read() & 0x0F;
    uint32_t payload = 0;
    for (int i = 0; i < 4; i++) {
      payload = (payload << 8) | Serial.read();
    }
    // Serial.print("RX CMD: 0b"); Serial.print(cmd, BIN);
    // Serial.print(", Payload: 0b"); Serial.println(payload, BIN);
    processCommand(cmd, payload);
  }

  // timeForSerialCheck = micros() - loopStart;

  // --- Projection loop ---
  if (currentMode == MODE_PROJECT && isProjecting && fileSize > 2) {
    static uint32_t lastUpdateTime = 0;
    static uint16_t valA, valB;

    if (micros() - lastUpdateTime >= currentDelayUs) {

      // timeForIfStatement = micros() - loopStart - timeForSerialCheck;

      if (playbackFile.available() >= int(sizeof(valA) + sizeof(valB))) {
        playbackFile.read((byte*)&valA, sizeof(valA));
        playbackFile.read((byte*)&valB, sizeof(valB));
        bytesRead += sizeof(valA) + sizeof(valB);
        bytesRead %= fileSize; // Wrap around

        // timeForBinaryRead = micros() - loopStart - timeForSerialCheck - timeForIfStatement;

        updateDAC(valA, valB);

        // timeForDACUpdate = micros() - loopStart - timeForSerialCheck - timeForIfStatement - timeForBinaryRead;

      } else {
        playbackFile.seek(0); // Loop to start
        bytesRead = 0;
      }

      lastUpdateTime = micros();
    }
  }
  // i++; 
  // if (i % 1000000 == 0 && isProjecting && currentMode == MODE_PROJECT) {
  //   Serial.print("Loop time (µs): "); Serial.print(micros() - loopStart);
  //   Serial.print("µs | Serial check: "); Serial.print(timeForSerialCheck);
  //   Serial.print("µs | Binary read: "); Serial.print(timeForBinaryRead);
  //   Serial.print("µs | DAC update: "); Serial.print(timeForDACUpdate);
  // }
}

// We are seeing a loop every 4 micro seconds (250kHz) --- ~4 microseconds are spent on updateDAC()
// We have ~6000 points in the file, so we loop ~0.025s (40 loops per second)


// Why we see distortion:
// The current XYZ -> (theta, phi) conversion assumes that r will stay constant, which is of course bullshit.