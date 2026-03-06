#pragma once

#include "HardwareAbstraction.hpp"
#include "../debug/Debug.hpp"

class Teensy4HAL : public HardwareAbstraction {
public:
    void enterLowPowerMode() override;
    void exitLowPowerMode() override;
    void rebootToBootloader() override;
    void systemReset() override;
};
