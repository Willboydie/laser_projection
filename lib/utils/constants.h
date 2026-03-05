#ifndef CONSTANTS_H
#define CONSTANTS_H

// System configuration constants
#define BUFFER_SIZE 512  // buffer must be a multiple of 512 for sdFatSdioEX
#define RECORD_SIZE 5  // 5 bytes per record: 2 bytes X + 2 bytes Y + 1 byte laser state

#define SYNC 10
#define LDAC 9
#define CLR  8
#define LASER_PIN 7 // Must be a PWM-capable pin

#define BAUDRATE 115200
#define DUTY_CYCLE_FREQUENCY 16000
#endif
