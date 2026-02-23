#pragma once

#include <iostream>
#include "Constants.hpp"

class Comms {

private:

    bool tcpInitialized = false;
    bool tcpConnected = false;

public:

    bool init();
    bool sendData(uint8_t msg);
    bool dataAvailable();
    void extractCommandAndPayload(uint8_t* data, uint32_t length);
    bool readPathData(uint8_t* data, uint32_t length);
    void endTCP();

private:

    bool setupTCP();
    bool connectToTCPServer();
    bool closeTCPServerConnection();
    void readCmdData(uint8_t* cmd);
    void readPayloadData(uint8_t* payload);
}

