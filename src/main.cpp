#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <DAC.h>
#include <TTL.h>

// ======== CONFIGURATION ========
#define WRITE_CMD    0b00000001
#define PROJECT_CMD  0b00000010
#define SAVE_CMD     0b00000100
#define PLAY_CMD     0b00001001
#define PAUSE_CMD    0b00001010
#define RESET_CMD    0b00001111

#define SD_CS_PIN BUILTIN_SDCARD    // SD card chip select
#define RECORD_SIZE 5               // 2 bytes valA + 2 bytes valB + 1 byte laser_on
#define COMMAND_SIZE 1              // 1 byte command

// ======== GLOBALS ========
enum Mode { MODE_WRITE, MODE_PROJECT };
Mode currentMode = MODE_WRITE;
bool isProjecting = false;

// Track file positions
uint32_t bytesWritten = 0;  // Total bytes written in WRITE mode
uint32_t bytesRead = 0;     // Current read position in PROJECT mode
uint32_t fileSize = 0;      // Total file size (bytes)

File playbackFile;          // File handle for playback
File writeFile;             // File handle for writing (kept open)
uint32_t currentDelayUs = 0; // Default delay for playback (none)


// ======== SETUP HELPER FUNCTIONS ========
void updateFileSize() {
  File tempFile = SD.open("data.bin", FILE_READ);
  if (tempFile) {
    fileSize = tempFile.size();
    tempFile.close();
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
uint32_t calculateDelayUs(uint8_t payload[RECORD_SIZE]) {
  uint8_t freq_KHz = payload[0];
  
  if (freq_KHz >= 250) {
    Serial.println("PLAY: 250KHz (full speed).");
    return 0;  // No delay for full speed
  } else if (freq_KHz == 0) {
    Serial.println("PLAY: 1KHz (slowest).");
    return 1000;  // 1 second delay
  } else {
    Serial.print("PLAY: "); Serial.print(freq_KHz); 
    Serial.println("KHz.");
    // Calculate delay in microseconds: 1,000,000 / freq_KHz
    return 1000 / freq_KHz;
  }
}

void wipeFile() {
  // Close write file if open
  if (writeFile) {
    writeFile.close();
  }
  
  SD.remove("data.bin");
  writeFile = SD.open("data.bin", FILE_WRITE);
  if (writeFile) {
    bytesWritten = 0;
    fileSize = 0;
    Serial.println("SD: File wiped.");
  } else {
    Serial.println("SD ERROR: Failed to wipe file!");
  }
}

// ======== COMMAND PROCESSING ========
void processCommand(uint8_t cmd, uint8_t payload[RECORD_SIZE]) {

  if (cmd == WRITE_CMD || cmd == PROJECT_CMD) {
    if (playbackFile) {
      playbackFile.close(); // Close file on mode switch
    }
    if (writeFile) {
      writeFile.close(); // Close write file on mode switch
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
      // --- In WRITE mode, save the 5-byte payload directly to SD ---
      if (currentMode == MODE_WRITE) {
        // Save the 5 bytes directly to SD
        if (writeFile) {
          size_t w = writeFile.write(payload, RECORD_SIZE);
          writeFile.flush();
          if (w == RECORD_SIZE) { 
            bytesWritten += RECORD_SIZE; 
            fileSize = bytesWritten; 
          } else {
            Serial.println("SD ERROR: Write failed!");
          }
        } else {
          Serial.println("SD ERROR: Write file not open!");
        }
      }
      break;

    // --- Project mode commands ---
    case PLAY_CMD:
      if (currentMode == MODE_PROJECT) {
        // Only update file size if we don't have a valid size or if file might have changed
        if (fileSize == 0) {
          updateFileSize();
        }
        if (fileSize < RECORD_SIZE) {
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

uint32_t loopCounter = 0;
uint32_t loopStart;
uint32_t timeForSerialCheck;
uint32_t timeForBinaryRead;
uint32_t timeForDACUpdate;
uint32_t timeForIfStatement;
bool laser_on = true;

uint8_t cmd;
uint8_t payload[RECORD_SIZE];

// ======== MAIN LOOP ========
void loop() {

  loopStart = micros();

  // --- Check for incoming 6-byte packet (1 byte command + 5 bytes payload) ---
  if (Serial.available() >= COMMAND_SIZE + RECORD_SIZE) {
    cmd = Serial.read();       // first byte

    // Read all 5 payload bytes
    for (int i = 0; i < RECORD_SIZE; i++) {
        if (Serial.available() > 0) {
            payload[i] = Serial.read();
        } else {
            // Buffer underrun - discard partial packet
            Serial.println("ERROR: Serial buffer underrun");
            break;
        }
    }

    processCommand(cmd, payload);
  }

  timeForSerialCheck = micros() - loopStart;

  // --- Projection loop ---
  if (currentMode == MODE_PROJECT && isProjecting && fileSize >= RECORD_SIZE) {
    static uint32_t lastUpdateTime = 0;
    static uint16_t valA, valB;
    static uint8_t on_off_signal;

    uint32_t currentTime = micros();
    // Handle micros() overflow (wraps every ~70 minutes)
    uint32_t elapsedTime = (currentTime >= lastUpdateTime) ? 
                          (currentTime - lastUpdateTime) : 
                          (0xFFFFFFFF - lastUpdateTime + currentTime + 1);
    
    if (elapsedTime >= currentDelayUs) {

      timeForIfStatement = micros() - loopStart - timeForSerialCheck;

      if (playbackFile.available() >= RECORD_SIZE) {
        playbackFile.read((byte*)&valA, sizeof(valA));
        playbackFile.read((byte*)&valB, sizeof(valB));
        playbackFile.read((byte*)&on_off_signal, sizeof(on_off_signal));
        bytesRead += RECORD_SIZE;
        bytesRead %= fileSize; // Wrap around

        timeForBinaryRead = micros() - loopStart - timeForSerialCheck - timeForIfStatement;

        // Only toggle laser if state actually changed
        if (laser_on != on_off_signal) {
          if (on_off_signal) {
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

      lastUpdateTime = currentTime;
    }
  }
  
  loopCounter++; 
  // Only print debug info every 10 million loops to reduce Serial overhead
  if (loopCounter % 10000000 == 0 && isProjecting && currentMode == MODE_PROJECT) {
    Serial.print("Loop time (µs): "); Serial.print(micros() - loopStart);
    Serial.print("µs | Serial check: "); Serial.print(timeForSerialCheck);
    Serial.print("µs | Binary read: "); Serial.print(timeForBinaryRead);
    Serial.print("µs | DAC update: "); Serial.print(timeForDACUpdate);
    Serial.println();
  }
}

// We are seeing a loop every 4 micro seconds (250kHz) --- ~4 microseconds are spent on updateDAC()