#pragma once

#include <Arduino.h>
#include "../file_manager/FileManager.hpp"
#include "../system_state/SystemState.hpp"
#include "../buffer/RotatingBuffer.hpp"
#include "../projector/Projector.hpp"
#include "../laser/Laser.hpp"
#include "../hal/HardwareAbstraction.hpp"

class System {
public:
    System(FileManager& fm, SystemState& ss, RotatingBuffer& rb,
           Projector& proj, Laser& laser, HardwareAbstraction& hal);
    ~System() = default;

    void init();
    void deinit();
    void enterWriteMode();
    void enterProjectMode();
    void playPause();
    void enable();
    void disable();
    void restart();
    void sleep();
    void wake();
    void activateBootloader();

private:
    FileManager& fileManager;
    SystemState& systemState;
    RotatingBuffer& buffer;
    Projector& projector;
    Laser& laser;
    HardwareAbstraction& hal;
};
