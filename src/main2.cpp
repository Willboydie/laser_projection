#include <Arduino.h>
#include <DAC.h>
#include "SdFat.h"

// SD card setup
SdFat sd;
SdFile dataFile;
const int chipSelect = BUILTIN_SDCARD;

// Mode flags
enum Mode { MODE_NONE, MODE_WRITE, MODE_PROJECT };
Mode currentMode = MODE_NONE;

uint32_t loopDelayMicros = 10; // default 10us between loops

// Buffer for incoming serial lines
String inputBuffer = "";
bool isPlaying = false;

void setup() {
    Serial.begin(115200);
    while (!Serial); // Wait for Serial connection
    initDAC(); // Initialize DAC

    if (!sd.begin(chipSelect)) {
        Serial.println("SD card init failed!");
    while (1);
    }
    Serial.println("Teensy ready.");
    }

void loop() {
    // Always listen for serial input
    checkSerial();

    // If in projection mode and playing
    if (currentMode == MODE_PROJECT && isPlaying) {
        uint32_t now = micros();
        if (now - lastUpdateMicros >= updateIntervalMicros) {
            updateDAC(dacValuesA[idx], dacValuesB[idx], updateIntervalMicros);
            idx++;
            if (idx >= pathSize) {
                idx = 0; // or stop playback
            }
            lastUpdateMicros = now;
        }
    }
}

void checkSerial() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n') {
            handleCommand(inputBuffer);
            inputBuffer = "";
        } else {
            inputBuffer += c;
            if (inputBuffer.length() > 128) {
                Serial.println("Input buffer overflow");
                inputBuffer = "";
            }
        }
    }
}

void handlePause() {
    isPlaying = false;
    Serial.println("Paused.");
}

void handlePlay() {
    isPlaying = true;
    Serial.println("Playing.");
}

void handleStop() {
    isPlaying = false;
    stopProjection();
    currentMode = MODE_NONE;
    Serial.println("Stopped.");
}


void handleCommand(const String& input) {
    if (input.startsWith("mode write")) {
        if (currentMode == MODE_PROJECT and isPlaying) {
            Serial.println("First stop projecting to enter write mode.");
        } else {
            currentMode = MODE_WRITE;
        }
        
    } else if (input.startsWith("mode project")) {
        if (currentMode == MODE_WRITE) {
            startProjection();
        }
        currentMode = MODE_PROJECT;

    } else if (input == "pause") {
        if (currentMode == MODE_PROJECT) {
            handlePause();
        } else {
            Serial.println("Cannot pause, not in project mode.");
        }

    } else if (input == "play") {
        if (currentMode == MODE_PROJECT) {
            handlePlay();
        } else {
            Serial.println("Cannot play, not in project mode.");
        }

    } else if (input.startsWith("a ")) {
        handleAngleCommand(input.substring(2));

    } else {
        Serial.println("Unknown command");
    }
}

void handleAngleCommand(const String& command) {
    
    uint16_t dacA = command.substring(0, spaceIndex).toInt();
    uint16_t dacB = command.substring(spaceIndex + 1).toInt();

    if (currentMode == MODE_WRITE) {
        writeToFile(dacA, dacB);
    } else if (currentMode == MODE_PROJECT) {
        // Update DAC with angles
        updateDAC(dacA, dacB, loopDelayMicros);
    } else {
        Serial.println("No mode set. Use 'mode write' or 'mode project'.");
    }
}


void writeToFile(uint16_t dacA, uint16_t dacB) {
    // Write angles to SD card
    if (dataFile.open("angles.txt", O_WRITE | O_CREAT | O_APPEND)) {
        dataFile.print(dacA);
        dataFile.print(" ");
        dataFile.println(dacB);
        dataFile.close();
    } else {
        Serial.println("Failed to open file for writing");
    }
}

void startProjection() {
    if (projectFile.open("angles.txt", O_READ)) {
        continue;
    } else {
        Serial.println("Failed to open angles.txt for projection.");
    }
}
void stopProjection() {
    if (projectFile.isOpen()) {
        projectFile.close();
        Serial.println("Stopped projection.");
    }
}
