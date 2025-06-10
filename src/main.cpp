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

#define SD_CS_PIN    10    // SD card chip select
#define DAC1_PIN     A14   // DAC0 (16-bit value A)
#define DAC2_PIN     A15   // DAC1 (16-bit value B)

// ======== GLOBALS ========
enum Mode { MODE_WRITE, MODE_PROJECT };
Mode currentMode = MODE_WRITE;
bool isProjecting = false;
File dataFile;

// Track file positions
uint32_t bytesWritten = 0;  // Total bytes written in WRITE mode
uint32_t bytesRead = 0;     // Current read position in PROJECT mode
uint32_t fileSize = 0;      // Total file size (bytes)

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

  initDac();
}

// ======== HELPER FUNCTIONS ========
void updateFileSize() {
  dataFile = SD.open("data.bin", FILE_READ);
  if (dataFile) {
    fileSize = dataFile.size();
    dataFile.close();
  } else {
    fileSize = 0;
  }
}

uint32_t calculateDelayUs(uint32_t payload) {
  if (payload == 0) {
    Serial.println("PLAY WARNING: No delay bits set. Using 1µs default.");
    return 1;
  }
  for (uint8_t bit = 0; bit < 32; bit++) {
    if (payload & (1 << (31 - bit))) { // LSB = bit 31
      uint32_t delay = 1;
      for (uint8_t i = 0; i < bit; i++) delay *= 10;
      Serial.print("PLAY: delay="); Serial.print(delay); 
      Serial.print("µs (bit "); Serial.print(bit); Serial.println(" set)");
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
void processCommand(uint32_t cmd, uint32_t payload) {

  if (cmd == WRITE_CMD || cmd == PROJECT_CMD) {
    if (playbackFile) {
      playbackFile.close(); // Close file on mode switch
    }
  }

  switch (cmd) {
    // --- Mode switches ---
    case WRITE_CMD:
      if (currentMode == MODE_PROJECT) {
        wipeFile(); // Wipe file when switching from PROJECT to WRITE
      }
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
      if (currentMode == MODE_WRITE) {
        uint16_t valA = (payload >> 16) & 0xFFFF;
        uint16_t valB = payload & 0xFFFF;
        dataFile = SD.open("data.bin", FILE_WRITE | O_APPEND);
        if (dataFile) {
          dataFile.write((byte*)&valA, sizeof(valA));
          dataFile.write((byte*)&valB, sizeof(valB));
          bytesWritten += sizeof(valA) + sizeof(valB);
          fileSize = bytesWritten;
          dataFile.close();
          Serial.print("SAVED: A=0x"); Serial.print(valA, HEX);
          Serial.print(", B=0x"); Serial.print(valB, HEX);
          Serial.print(" | Total bytes: "); Serial.println(fileSize);
        } else {
          Serial.println("SD ERROR: Failed to save data!");
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
        
        uint32_t delayUs = calculateDelayUs(payload);
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

// ======== MAIN LOOP ========
void loop() {
  // --- Check for incoming 36-bit command ---
  if (Serial.available() >= 5) {
    uint32_t cmd = Serial.read() & 0x0F;
    uint32_t payload = 0;
    for (int i = 0; i < 4; i++) {
      payload = (payload << 8) | Serial.read();
    }
    Serial.print("RX CMD: 0b"); Serial.print(cmd, BIN);
    Serial.print(", Payload: 0x"); Serial.println(payload, HEX);
    processCommand(cmd, payload);
  }

  // --- Projection loop ---
  if (currentMode == MODE_PROJECT && isProjecting && fileSize > 0) {
    static uint32_t lastUpdateTime = 0;
    static uint32_t currentDelayUs = 1000;
    static File playbackFile;
    static uint16_t valA, valB;

    if (micros() - lastUpdateTime >= currentDelayUs) {
      if (playbackFile.available() >= sizeof(valA) + sizeof(valB)) {
        playbackFile.read((byte*)&valA, sizeof(valA));
        playbackFile.read((byte*)&valB, sizeof(valB));
        bytesRead += sizeof(valA) + sizeof(valB);
        bytesRead %= fileSize; // Wrap around

        updateDAC(valA, valB);

        Serial.print("DAC: A=0x"); Serial.print(valA, HEX);
        Serial.print(", B=0x"); Serial.print(valB, HEX);
        Serial.print(" | Position: "); Serial.print(bytesRead);
        Serial.print("/"); Serial.print(fileSize);
        Serial.print(" ("); Serial.print((bytesRead * 100) / fileSize);
        Serial.println("%%)");
      } else {
        playbackFile.seek(0); // Loop to start
        bytesRead = 0;
      }
      lastUpdateTime = micros();
    }
  }
}



// Why we see distortion:

// The current XYZ -> (theta, phi) conversion assumes that r will stay constant, which is of course bullshit.