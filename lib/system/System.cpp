#include "System.hpp"

System::System(FileManager& fm, SystemState& ss, RotatingBuffer& rb,
               Projector& proj, Laser& laser, HardwareAbstraction& hal)
    : fileManager(fm)
    , systemState(ss)
    , buffer(rb)
    , projector(proj)
    , laser(laser)
    , hal(hal)
    {}

void System::init() {
    Serial.println("Initializing system...");
    if (!fileManager.init()) {
        Serial.println("FATAL: File system initialisation failed!");
        return;
    }
    projector.init();
    Serial.println("System initialized successfully");
}

void System::deinit() {
    Serial.println("Deinitializing system...");
    buffer.deinit();
    projector.deinit();
    fileManager.deinit();
    Serial.println("System deinitialized successfully");
}

void System::enterWriteMode() {
    if (!systemState.enabled) {
        Serial.println("ERROR: System is not enabled!");
        return;
    }
    if (systemState.mode == MODE_WRITE) {
        Serial.println("WARNING: Already in WRITE mode!");
        return;
    }
    if (fileManager.getFileSize() > 0) fileManager.wipeFile();
    if (fileManager.openForWrite()) {
        systemState.mode = MODE_WRITE;
        systemState.projecting = false;
        Serial.println("Ready to save records");
    }
}

void System::enterProjectMode() {
    if (!systemState.enabled) {
        Serial.println("ERROR: System is not enabled!");
        return;
    }
    if (systemState.mode == MODE_PROJECT) {
        Serial.println("WARNING: Already in PROJECT mode!");
        return;
    }
    if (fileManager.getFileSize() < RECORD_SIZE) {
        Serial.println("ERROR: Too few points in .bin file. Cannot enter project mode.");
        return;
    }
    if (fileManager.openForRead()) {
        systemState.mode = MODE_PROJECT;
        systemState.projecting = false;
        if (!buffer.init()) {
            Serial.println("ERROR: Failed to initialise rotating buffer");
            return;
        }
        Serial.println("Ready for playback");
    } else {
        Serial.println("ERROR: Failed to prepare for projection");
    }
}

void System::playPause() {
    if (!systemState.enabled) return;
    if (systemState.mode != MODE_PROJECT) {
        Serial.println("ERROR: Not in PROJECT mode");
        return;
    }
    
    if (systemState.projecting) {
        systemState.projecting = false;
        Serial.println("Projection paused");
    } else {
        systemState.projecting = true;
        Serial.println("Projection started");
    }
}

void System::enable() {
    if (systemState.enabled) return;

    systemState.enabled = true;
    systemState.mode = MODE_READY;
    Serial.println("System enabled. Select a WRITE or PROJECT mode to start.");
}

void System::disable() {
    if (!systemState.enabled) return;

    systemState.enabled = false;
    systemState.projecting = false;
    if (laser.isOn()) laser.off();
    buffer.deinit();
    Serial.println("System Disabled.");
}

void System::restart() {
    Serial.println("Restarting...");
    disable();
    deinit();
    init();
    Serial.println("Restart complete");
}

void System::sleep() {
    if (systemState.powerState == POWER_SLEEPING) {
        Serial.println("WARNING: Already sleeping");
        return;
    }

    Serial.println("Entering sleep mode...");
    disable();
    deinit();
    systemState.powerState = POWER_SLEEPING;
    hal.enterLowPowerMode();
    Serial.println("System is now sleeping");
}

void System::wake() {
    if (systemState.powerState != POWER_SLEEPING) {
        Serial.println("WARNING: System is not sleeping");
        return;
    }

    hal.exitLowPowerMode();
    systemState.powerState = POWER_AWAKE;
    init();
    Serial.println("System is awake. Enable to begin.");
}

void System::activateBootloader() {
    Serial.println("Entering bootloader mode...");
    disable();
    deinit();
    Serial.flush();
    hal.rebootToBootloader();
}
