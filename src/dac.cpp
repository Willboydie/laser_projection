// #include <SPI.h>
// #define SYNC 10
// #define LDAC 9
// #define CLR  8

// void sendSPICommand(uint8_t cmd_high, uint16_t cmd_low) {
//   digitalWrite(SYNC, LOW);
//   SPI.transfer(cmd_high);
//   SPI.transfer16(cmd_low);
//   digitalWrite(SYNC, HIGH);
// }

// void setup() {
//   Serial.begin(115200);
//   while (!Serial);

//   // Initialize pins
//   pinMode(SYNC, OUTPUT);
//   pinMode(LDAC, OUTPUT);
//   pinMode(CLR, OUTPUT);
//   digitalWrite(SYNC, HIGH);
//   digitalWrite(LDAC, HIGH);
//   digitalWrite(CLR, HIGH); // Disable reset

//   // Start SPI (MODE0!)
//   SPI.begin();
//   SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0)); // FIXED: MODE0
//   Serial.println("SPI initialized");

//   // Software reset (optional)
//   sendSPICommand(0b00101000, 0b0000000000000001); // Command 101 (reset), data=1

//   // Power up DAC A and B
//   sendSPICommand(0b00100000, 0b0000000000000011); // Command 100 (power up), data=3

//   // Enable internal reference (2.5V) + gain=2 (0-5V range)
//   sendSPICommand(0b00111000, 0b0000000000000001); // Command 111 (enable ref), data=1
//   Serial.println("Internal reference enabled, gain=2");

//   // Test: Set DAC-A to mid-scale (2.5V)
//   sendSPICommand(0b00011000, 0x8000); // Write+update DAC-A
//   Serial.println("DAC set to 0x8000 (2.5V)");
// }

// void loop() {}