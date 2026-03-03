#include "Teensy4HAL.hpp"
#include <Arduino.h>

void Teensy4HAL::enterLowPowerMode() {
    // Teensy 4.1 cannot enter deep sleep while USB Serial remains active.
    // Application-level power savings (disabling projector, laser, file system)
    // are handled by System::sleep() before this is called.
    //
    // For production use, consider:
    //   - Reducing CPU clock via set_arm_clock()
    //   - Gating unused peripheral clocks via CCM registers
    //   - Using ARM WFI in the idle loop for inter-interrupt savings
}

void Teensy4HAL::exitLowPowerMode() {
    // Restore any hardware-level changes made in enterLowPowerMode().
    // Currently a no-op since we rely on application-level sleep.
}

void Teensy4HAL::rebootToBootloader() {
    _reboot_Teensyduino_();
}

void Teensy4HAL::systemReset() {
    SCB_AIRCR = 0x05FA0004;
}
