#include "Teensy4HAL.hpp"
#include <Arduino.h>

void Teensy4HAL::enterLowPowerMode() {
    // Teensy 4.1 cannot enter deep sleep while USB Serial remains active.
    // Application-level power savings (disabling projector, laser, file system)
    // are handled by System::sleep() before this is called.
    //
    Serial.println("Entering low power mode...");
    Serial.flush();     // finish USB transfers
    asm("dsb");
    asm("wfi");         // sleep until serial detected
}

void Teensy4HAL::exitLowPowerMode() {
    Serial.println("CPU waking up...");
}

void Teensy4HAL::rebootToBootloader() {
    _reboot_Teensyduino_();
}

void Teensy4HAL::systemReset() {
    SCB_AIRCR = 0x05FA0004;
}
