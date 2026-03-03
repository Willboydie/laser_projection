#include <Arduino.h>
#include <SPI.h>
#include "../lib/dac_spi/DAC_SPI.hpp"
#include "../lib/laser/Laser.hpp"
#include "../lib/file_manager/FileManager.hpp"
#include "../lib/system_state/SystemState.hpp"
#include "../lib/interface/Interface.hpp"
#include "../lib/projector/Projector.hpp"
#include "../lib/utils/constants.h"


FileManager fileManager;
SystemState systemState;
RotatingBuffer buffer(&fileManager);
DAC_SPI dacSPI(SYNC, LDAC, CLR);
Laser laser(LASER_PIN);
Projector projector(&dacSPI, &laser, &systemState, &buffer);
Interface interface(&fileManager, &systemState, &projector);


void setup() {
  Serial.begin(BAUDRATE);
  while (!Serial);

  if (!fileManager.init()) {
    Serial.println("FATAL: File system initialization failed!");
    return;
  }

  projector.init();
}


void loop() {

  static unsigned long lastCommandTime = 0;
  if (millis() - lastCommandTime > 2000) {
    lastCommandTime = millis();
    interface.listen();
  }
  
  if ((systemState.mode == MODE_PROJECT) && systemState.projecting) {
    projector.update();
  }

  static unsigned long printInterval = 1000;
  static unsigned long lastPrintTime = 0;
  if ((millis() - lastPrintTime > printInterval) && (systemState.projecting)) {
    Serial.print("Frequency: ");
    Serial.print(1000.0 * projector.getPointsProjected() / printInterval);  // Convert microseconds to Hz
    Serial.println(" pps");
    lastPrintTime = millis();
    projector.setPointsProjected(0);
  }
}