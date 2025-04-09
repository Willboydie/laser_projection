#include <SPI.h>
#include <Arduino.h>
#include <math.h>

#define SYNC 10
#define LDAC 9
#define CLR  8

void sendSPICommand(uint8_t cmd, uint16_t data) {
  digitalWrite(SYNC, LOW);
  SPI.transfer(cmd);
  SPI.transfer16(data); // Send 16-bit data
  digitalWrite(SYNC, HIGH);
}

void setup() {
    Serial.begin(115200);
    while (!Serial);

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

    // Power up DAC A and B
    sendSPICommand(0b00100000, 0b0000000000000011); // Command 100 (power up), data=3
    delay(10); // Wait for power up to complete
    Serial.println("DAC A and B powered up");

  // Software reset (optional)
    sendSPICommand(0b00101000, 0b0000000000000001); // Command 101 (reset), data=1
    delay(10); // Wait for reset to complete
    Serial.println("Software reset complete");

    // Enable internal reference (2.5V) + gain=2 (0-5V range)
    sendSPICommand(0b00111000, 0b0000000000000001); // Command 111 (enable ref), data=1
    delay(10); // Wait for reference to stabilize
    Serial.println("Internal reference enabled - gain=2");

    // Test: Set DAC-A to mid-scale (2.5V)
    sendSPICommand(0b00011000, 50000); // Write+update DAC A

    // midscales. A=32755, B=32711 (0V)
    // 44 diff (A higher) - lets test

    sendSPICommand(0b00011001, 50000); // write and update DAC B
}

int i = 0;

void loop() {

    // deliver a sine wave y = sin(x) * 32767 + 32767
    // 350 Hz speed
    for (int i = 0; i < 360; i++) {
        float x = (float)i * (2.0 * M_PI / 360.0); // Convert to radians
        int y1 = (int)(sin(x) * 25000 + 25000); // Scale to DAC range
        int y2 = (int)(cos(x) * 25000 + 25000);
        sendSPICommand(0b00011000, y1); // Write+update DAC A
        sendSPICommand(0b00011001, y2); // Write+update DAC B
        delay(100); // Adjust delay for desired frequency
    }
}