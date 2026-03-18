#ifndef CONSTANTS_H
#define CONSTANTS_H

// System configuration constants
#define BUFFER_SIZE 512  // buffer must be a multiple of 512 for sdFatSdioEX
#define RECORD_SIZE 5  // 5 bytes per record: 2 bytes X + 2 bytes Y + 1 byte laser state

#define SYNC 10
#define LDAC 9
#define CLR  8
#define LASER_PIN 36 // Must be a PWM-capable pin

#define DUTY_CYCLE_FREQUENCY 16000
#define INITIAL_LASER_DUTY_CYCLE 0.5f

#define BAUDRATE 115200
#define STATUS_PACKET_INTERVAL 100 // in milliseconds

#define DEFAULT_DEBUG_SETTING true

#define LOOPBACK_RECORD_SIZE 4    // 2 bytes X + 2 bytes Y (laser state not on SPI)
#define LOOPBACK_BUFFER_SIZE 512  // points per write buffer (must be multiple of 512 for SD)
#endif
