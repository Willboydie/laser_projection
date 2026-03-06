#include <Arduino.h>
#include <SPI.h>
#include "../lib/dac_spi/DAC_SPI.hpp"
#include "../lib/laser/Laser.hpp"
#include "../lib/file_manager/FileManager.hpp"
#include "../lib/system_state/SystemState.hpp"
#include "../lib/interface/Interface.hpp"
#include "../lib/projector/Projector.hpp"
#include "../lib/system/System.hpp"
#include "../lib/hal/Teensy4HAL.hpp"
#include "../lib/utils/constants.h"

FileManager fileManager;
SystemState systemState;
RotatingBuffer buffer(fileManager);
DAC_SPI dacSPI(SYNC, LDAC, CLR);
Laser laser(LASER_PIN, INITIAL_LASER_DUTY_CYCLE);
Projector projector(dacSPI, laser, systemState, buffer);
Teensy4HAL hal;
System sys(fileManager, systemState, buffer, projector, laser, hal);
Interface interface(fileManager, systemState, projector, sys, laser);

void setup() {
  Serial.begin(BAUDRATE);
  while (!Serial);
  sys.init();
}

void loop() {

  static unsigned long lastCommandTime = 0;
  if (millis() - lastCommandTime > 50) {
    lastCommandTime = millis();
    interface.listen();
  }

  if (systemState.powerState == POWER_SLEEPING) return;
  
  if ((systemState.enabled) && (systemState.mode == MODE_PROJECT)) {
    projector.update();
  }
}