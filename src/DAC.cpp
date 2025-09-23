#include <DAC.h>

void fastSPITransfer(uint8_t data) {
  LPSPI1_TDR = data;
  while (!(LPSPI1_SR & LPSPI_SR_TDF)) {}
}

void sendSPICommand(uint8_t cmd, uint16_t data) {
  GPIO2_DR_CLEAR = (1 << 20); // Set bit 20 of GPIO2 to 0 (LOW) (sets pin 10 to LOW)
  fastSPITransfer(cmd);
  fastSPITransfer(highByte(data));
  fastSPITransfer(lowByte(data));
  GPIO2_DR_SET = (1 << 20); // Set bit 20 of GPIO2 to 1 (HIGH) (sets pin 10 to HIGH)
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
    SPI.beginTransaction(SPISettings(100000000, MSBFIRST, SPI_MODE0));
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
    delay(100); // Wait for DAC to settle
}
