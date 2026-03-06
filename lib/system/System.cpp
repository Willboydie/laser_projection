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
    Debug::msg("Initializing system...");
    if (!fileManager.init()) {
        Debug::msg("FATAL: File system initialisation failed!");
        return;
    }
    projector.init();
    Debug::msg("System initialized successfully");
}

void System::deinit() {
    Debug::msg("Deinitializing system...");
    buffer.deinit();
    projector.deinit();
    fileManager.deinit();
    Debug::msg("System deinitialized successfully");
}

void System::enterWriteMode() {
    if (!systemState.enabled) {
        Debug::msg("ERROR: System is not enabled!");
        return;
    }
    if (systemState.mode == MODE_WRITE) {
        Debug::msg("WARNING: Already in WRITE mode!");
        return;
    }
    if (fileManager.getFileSize() > 0) fileManager.wipeFile();
    if (fileManager.openForWrite()) {
        systemState.mode = MODE_WRITE;
        systemState.projecting = false;
        Debug::msg("Ready to save records");
    }
}

void System::enterProjectMode() {
    if (!systemState.enabled) {
        Debug::msg("ERROR: System is not enabled!");
        return;
    }
    if (systemState.mode == MODE_PROJECT) {
        Debug::msg("WARNING: Already in PROJECT mode!");
        return;
    }
    if (fileManager.getFileSize() < RECORD_SIZE) {
        Debug::msg("ERROR: Too few points in .bin file. Cannot enter project mode.");
        return;
    }
    if (fileManager.openForRead()) {
        systemState.mode = MODE_PROJECT;
        systemState.projecting = false;
        if (!buffer.init()) {
            Debug::msg("ERROR: Failed to initialise rotating buffer");
            return;
        }
        Debug::msg("Ready for playback");
    } else {
        Debug::msg("ERROR: Failed to prepare for projection");
    }
}

void System::playPause() {
    if (!systemState.enabled) return;
    if (systemState.mode != MODE_PROJECT) {
        Debug::msg("ERROR: Not in PROJECT mode");
        return;
    }
    
    if (systemState.projecting) {
        systemState.projecting = false;
        laser.off();
        Debug::msg("Projection paused");
    } else {
        systemState.projecting = true;
        Debug::msg("Projection started");
    }
}

void System::enable() {
    if (systemState.enabled) return;

    systemState.enabled = true;
    systemState.mode = MODE_READY;
    Debug::msg("System enabled. Select a WRITE or PROJECT mode to start.");
}

void System::disable() {
    if (!systemState.enabled) return;

    systemState.enabled = false;
    systemState.projecting = false;
    systemState.mode = MODE_READY;
    if (laser.isOn()) laser.off();
    buffer.deinit();
    Debug::msg("System Disabled.");
}

void System::restart() {
    Debug::msg("Restarting...");
    disable();
    deinit();
    init();
    Debug::msg("Restart complete");
}

void System::sleep() {
    if (systemState.powerState == POWER_SLEEPING) {
        Debug::msg("WARNING: Already sleeping");
        return;
    }

    Debug::msg("Entering sleep mode...");
    disable();
    deinit();
    systemState.powerState = POWER_SLEEPING;
    hal.enterLowPowerMode();
    Debug::msg("System is now sleeping");
}

void System::wake() {
    if (systemState.powerState != POWER_SLEEPING) {
        Debug::msg("WARNING: System is not sleeping");
        return;
    }

    hal.exitLowPowerMode();
    systemState.powerState = POWER_AWAKE;
    init();
    Debug::msg("System is awake. Enable to begin.");
}

void System::activateBootloader() {
    Debug::msg("Entering bootloader mode...");
    disable();
    deinit();
    Serial.flush();
    hal.rebootToBootloader();
}