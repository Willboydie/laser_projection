#include <iostream>
#include "CommandProcessor.hpp"
#include "DataManager.hpp"
#include "SystemState.hpp"
#include "Projector.hpp"
#include "Constants.hpp"

// ======== GLOBALS ========
DataManager dataManager;
SystemState systemState;
Projector projector;
Comms comms;
CommandProcessor commandProcessor(&dataManager, &systemState, &projector);

using namespace std::chrono::steady_clock;

// ======== SETUP ========
int setup() {

    // init comms
    if (!comms.init()) {
        std::cerr << "FATAL: Comms initialization failed!" << std::endl;
        return -1;
    }

    // init data manager
    if (!dataManager.init()) {
        std::cerr << "FATAL: Data manager initialization failed!" << std::endl;
        return -1;
    }

    // init system state
    if (!systemState.init()) {
        std::cerr << "FATAL: System state initialization failed!" << std::endl;
        return -1;
    }

    return 0; // success
}

// ======== MAIN LOOP ========
int main() {
    // setup
    if (setup() != 0) {
        std::cerr << "FATAL: Setup failed!" << std::endl;
        return -1;
    }

    // Command processing variables
    uint8_t cmd;
    uint8_t payload[RECORD_SIZE];
    uint32_t loopCounter = 0;


    // main loop
    auto startTime = now();
    while (true) {
        if (loopCounter % 5000 == 0) {
            if (comms.dataAvailable() < RECORD_SIZE) {
                 // Read payload bytes
                comms.readCmdData(&cmd);
                comms.readPayloadData(&payload);
                commandProcessor.processCommand(cmd, payload);
            }
        }

        if (systemState.getMode() == SystemState::MODE_PROJECT && systemState.isCurrentlyProjecting()) {
            auto elapsed = now() - startTime;
            if (duration_cast<microseconds>(elapsed).count() > systemState.getDelay()) {
                projector.update();
                startTime = now();
            }
        }

        loopCounter = (loopCounter + 1) % 4000000000; // Prevent overflow
    }
}