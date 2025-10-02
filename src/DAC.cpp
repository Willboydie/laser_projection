#include <DAC.h>
#include <SPI.h>


void SPITransfer24(uint32_t frame) {
  SPI.transfer16((frame >> 8) & 0xFFFF);  // Send bits 23-8 (16 bits)
  SPI.transfer(frame & 0xFF);             // Send bits 7-0 (8 bits)
}


void sendSPICommand(uint32_t frame) {
  digitalWriteFast(SYNC, LOW);
  SPITransfer24(frame);
  digitalWriteFast(SYNC, HIGH);
}


void FAST_DAC_UPDATE(uint16_t A, uint16_t B) {
  sendSPICommand((0b00011000 << 16) | ((A) & 0xFFFF));
  sendSPICommand((0b00011001 << 16) | ((B) & 0xFFFF));
}

// Uses SPI.h
void initSPIControl() {
    // Initialize SPI with standard library
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV2);  // ~30 MHz clock (clock_div4 is slower)
    SPI.setDataMode(SPI_MODE0);           // CPOL=0, CPHA=0
    SPI.setBitOrder(MSBFIRST);            // Most significant bit first
    
    // Initialize control pins
    pinMode(SYNC, OUTPUT);
    pinMode(LDAC, OUTPUT);
    pinMode(CLR, OUTPUT);
    digitalWrite(SYNC, HIGH);
    digitalWrite(LDAC, HIGH);
    digitalWrite(CLR, HIGH);
    
    Serial.println("SPI Control initialized (using SPI.h library)");
}


void initDAC() {
    
    initSPIControl();

    // Power down DAC A and B
    sendSPICommand(0b001000000000000000100011); // Command 100 (power down), data=0x0023
    delay(100); // Wait for power down to complete 

    // Power up DAC A and B
    sendSPICommand(0b001000000000000000000011); // Command 100 (power up), data=0x0003
    delay(100); // Wait for power up to complete
    Serial.println("DAC A and B powered up");

    // Software reset (optional)
    sendSPICommand(0b001010000000000000000001); // Command 101 (reset), data=0x0001
    delay(100); // Wait for reset to complete
    Serial.println("Software reset complete");

    // Enable internal reference (2.5V) + gain=2 (0-5V range)
    sendSPICommand(0b001110000000000000000001); // Command 111 (enable ref), data=0x0001
    delay(10); // Wait for reference to stabilize
    Serial.println("Internal reference enabled");

    delay(100);
    uint16_t A = 50000;
    uint16_t B = 25000;

    FAST_DAC_UPDATE(A, B);
    Serial.print("DAC A and B set to ");
    Serial.print(A);
    Serial.print(" & ");
    Serial.println(B);

}

