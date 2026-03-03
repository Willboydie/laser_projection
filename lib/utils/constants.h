#ifndef CONSTANTS_H
#define CONSTANTS_H

// System configuration constants
#define BUFFER_SIZE 512  // buffer must be a multiple of 512 for sdFatSdioEX
#define RECORD_SIZE 5  // 5 bytes per record: 2 bytes X + 2 bytes Y + 1 byte laser state
#define UPLOAD_CHUNK_SIZE 510 // 512 - 2 for command
#define PLAYSPEED_SIZE 2

#define SYNC 10
#define LDAC 9
#define CLR  8

#endif // CONSTANTS_H
