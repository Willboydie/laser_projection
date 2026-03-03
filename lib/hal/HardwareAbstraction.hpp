#pragma once

class HardwareAbstraction {
public:
    virtual ~HardwareAbstraction() = default;

    virtual void enterLowPowerMode() = 0;
    virtual void exitLowPowerMode() = 0;
    virtual void rebootToBootloader() = 0;
    virtual void systemReset() = 0;
};
