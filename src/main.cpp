#include <Arduino.h>
#include <SPI.h>
#include <DAC.h>
#include <TTL.h>
#include "FileManager.h"
#include "SystemState.h"
#include "CommandProcessor.h"
#include "Projector.h"
#include "Constants.h"

// ======== GLOBALS ========
FileManager fileManager;
SystemState systemState;
Projector projector;
CommandProcessor commandProcessor(&fileManager, &systemState, &projector);

// Command processing variables
uint8_t cmd;
uint8_t payload[RECORD_SIZE];
uint32_t loopCounter = 0;


// ======== SETUP ========
void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Initialize hardware
  initTTL();
  initDAC();
  
  // Initialize file system
  if (!fileManager.initialize()) {
    Serial.println("FATAL: File system initialization failed!");
    while (1);
  }
  
  // Initialize projector with file manager reference
  projector.init(&fileManager, &systemState);
  
  Serial.println("System initialized successfully");
  systemState.printStatus();
}

// ======== MAIN LOOP ========

static uint32_t startTime = micros();
void loop() {
  // Check for serial commands (less frequently to reduce overhead)
  if (loopCounter % 2000 == 0) {
    if (Serial.available() >= 6) { // 1 command + 5 payload bytes
      cmd = Serial.read();
      
      // Read payload bytes
      for (int i = 0; i < 5; i++) {
        payload[i] = Serial.read();
      }
      
      // Process command
      commandProcessor.processCommand(cmd, payload);
    }
  }
  
  // Update projector if in project mode
  if ((systemState.getMode() == SystemState::MODE_PROJECT) && systemState.isCurrentlyProjecting()) {
    projector.update();
  }
  
  loopCounter++;
  
  // Debug info (less frequent)
  if ((loopCounter % 1000000 == 0) && (systemState.isCurrentlyProjecting())) {
    uint32_t currentTime = micros();
    uint32_t elapsedTime = (currentTime >= startTime) ? 
                          (currentTime - startTime) : 
                          (0xFFFFFFFF - startTime + currentTime + 1);
    
    Serial.print("Frequency: ");
    Serial.print(1000000.0 / elapsedTime);  // Convert microseconds to Hz
    Serial.println("Hz");
    Serial.print("Average DMA fill time: ");
    Serial.print(projector.getDMACompletionCount() > 0 ? 
        String(projector.getDMAFillTime() / projector.getDMACompletionCount()) : 
        "None Completed");
    Serial.println("(μs)");
    Serial.println();
    
    loopCounter = 0;
    startTime = currentTime;
  }
}

