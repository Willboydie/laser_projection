#include <SPI.h>
#include <Arduino.h>

#define MAX_POINTS 5000 // Maximum number of points in an array

#define SYNC 10
#define LDAC 9
#define CLR  8

struct DACcmd {
  uint16_t A;
  uint16_t B; 
};
DACcmd dacCmds[MAX_POINTS];

void sendSPICommand(uint8_t cmd, uint16_t data) {
  digitalWrite(SYNC, LOW);
  SPI.transfer(cmd);
  SPI.transfer16(data); // Send 16-bit data
  digitalWrite(SYNC, HIGH);
}

void initDAC() {

    // Initialize pins
    pinMode(SYNC, OUTPUT);
    pinMode(LDAC, OUTPUT);
    pinMode(CLR, OUTPUT);
    digitalWrite(SYNC, HIGH);
    digitalWrite(LDAC, HIGH);
    digitalWrite(CLR, HIGH); // Disable reset

    // Start SPI (MODE0!)
    SPI.begin();
    SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
    Serial.println("SPI initialized");

    // Power down DAC A and B
    sendSPICommand(0b00100000, 0b00000000000100011); // Command 100 (power down), data=0
    delay(100); // Wait for power down to complete 

    // Power up DAC A and B
    sendSPICommand(0b00100000, 0b0000000000000011); // Command 100 (power up), data=3
    delay(100); // Wait for power up to complete
    Serial.println("DAC A and B powered up");

    // Software reset (optional)
    sendSPICommand(0b00101000, 0b0000000000000001); // Command 101 (reset), data=1
    delay(100); // Wait for reset to complete
    Serial.println("Software reset complete");

    // Enable internal reference (2.5V) + gain=2 (0-5V range)
    sendSPICommand(0b00111000, 0b0000000000000001); // Command 111 (enable ref), data=1
    delay(10); // Wait for reference to stabilize
    Serial.println("Internal reference enabled");


    sendSPICommand(0b00011111, (int)65535/2.0); // Set Dac A+B to 0V
    delay(3000); // Wait for DAC to settle
}


void updateDAC(uint16_t A, uint16_t B, int delay_time) {
    // Ensure values are within 0-65535 range
    A = constrain(A, 0, 65535);
    B = constrain(B, 0, 65535);

    // Send the command to the DAC
    sendSPICommand(0b00000000, A); // Write DAC A
    sendSPICommand(0b00000001, B); // Write DAC B
    sendSPICommand(0b00001111, 0b0000000000000000); // Update Dac A+B

    delayMicroseconds(delay_time);
}