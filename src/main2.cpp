#include <Arduino.h>
#include "SdFat.h"

// SD card setup
SdFat sd;
SdFile dataFile;
const int chipSelect = BUILTIN_SDCARD;

// Mode flags
enum Mode { MODE_NONE, MODE_SAVE, MODE_PROJECT, MODE_PROJECT_FILE };
Mode currentMode = MODE_NONE;

String currentFilename = "";
uint32_t loopDelayMicros = 10; // default 10us between loops

// Buffer for incoming serial lines
String inputBuffer = "";

// Function prototypes
void handleCommand(const String& line);
void handleAngleCommand(const String& arg1, const String& arg2);
void projectToDAC(uint16_t angleA, uint16_t angleB);
void projectFile(const String& filename, uint32_t delayMicros);

void setup() {
  Serial.begin(115200);
  while (!Serial) { } // Wait for Serial connection

  if (!sd.begin(chipSelect)) {
    Serial.println("SD card init failed!");
    while (1);
  }
  Serial.println("Teensy ready.");
}

void loop() {
  // Read serial input line by line
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') { // Newline indicates end of command
      handleCommand(inputBuffer);
      inputBuffer = "";
    } else {
      inputBuffer += c;
    }
  }
}

void handleCommand(const String& line) {
  if (line.length() == 0) return;

  char command = line.charAt(0);
  if (command == 's') {
    // Save mode: s filename
    int sep = line.indexOf(' ');
    if (sep > 0) {
      currentFilename = line.substring(sep + 1);
      currentMode = MODE_SAVE;
      Serial.print("Now saving angles to ");
      Serial.println(currentFilename);
    } else {
      Serial.println("Error: missing filename");
    }

  } else if (command == 'p') {
    // Project mode: p [filename] [delay]
    int firstSpace = line.indexOf(' ');
    if (firstSpace < 0) {
      currentMode = MODE_PROJECT;
      Serial.println("Now projecting angles live.");
      return;
    }

    int secondSpace = line.indexOf(' ', firstSpace + 1);
    String filename = line.substring(firstSpace + 1, secondSpace > 0 ? secondSpace : line.length());
    uint32_t delayMicros = 10;
    if (secondSpace > 0) {
      delayMicros = line.substring(secondSpace + 1).toInt();
      if (delayMicros == 0) delayMicros = 10;
    }

    if (!sd.exists(filename.c_str())) {
      Serial.print("Error: file ");
      Serial.print(filename);
      Serial.println(" not found.");
      return;
    }

    currentMode = MODE_PROJECT_FILE;
    Serial.print("Projecting from file ");
    Serial.print(filename);
    Serial.print(" with ");
    Serial.print(delayMicros);
    Serial.println(" us delay.");
    projectFile(filename, delayMicros);

  } else if (command == 'a') {
    // a 16bit_binary 16bit_binary
    int firstSpace = line.indexOf(' ');
    int secondSpace = line.indexOf(' ', firstSpace + 1);
    if (firstSpace > 0 && secondSpace > firstSpace) {
      String arg1 = line.substring(firstSpace + 1, secondSpace);
      String arg2 = line.substring(secondSpace + 1);
      handleAngleCommand(arg1, arg2);
    } else {
      Serial.println("Error: malformed 'a' command.");
    }

  } else {
    Serial.print("Unknown command: ");
    Serial.println(line);
  }
}

void handleAngleCommand(const String& arg1, const String& arg2) {

    uint16_t angleA = strtol(arg1.c_str(), nullptr, 10);
    uint16_t angleB = strtol(arg2.c_str(), nullptr, 10);

    if (angleA > 65535 || angleB > 65535) {
        Serial.println("Error: angles must be 16-bit values.");
        return;
    }
    if (currentMode == MODE_SAVE) {
        if (currentFilename.length() == 0) {
            Serial.println("Error: no filename specified.");
            return;
        }

        if (!dataFile.open(currentFilename.c_str(), FILE_WRITE)) {
            Serial.println("Error: can't open file for writing.");
            return;
        }

        dataFile.print(arg1);
        dataFile.print(" ");
        dataFile.println(arg2);
        dataFile.close();

        Serial.println("Angle pair saved.");

    } else if (currentMode == MODE_PROJECT) {
        projectToDAC(angleA, angleB);

    } else {
        Serial.println("Error: no mode selected.");
    }
}

void projectToDAC(uint16_t angleA, uint16_t angleB) {
  // Replace this with your DAC driver code
    Serial.print("Projecting A:");
    Serial.print(angleA);
    Serial.print(" B:");
    Serial.println(angleB);
}

void projectFile(const String& filename, uint32_t delayMicros) {
    if (!dataFile.open(filename.c_str(), FILE_READ)) {
        Serial.println("Error: failed to open file for reading.");
        return;
    }

    String line;
    while (dataFile.available()) {
        char c = dataFile.read();
        if (c == '\n') {    // This is what signifies the end of a command
            handleCommand(line);
            line = "";
            delayMicroseconds(delayMicros);
        } else {
        line += c;
        }
    }

  dataFile.close();
  Serial.println("Finished projecting file.");
}
