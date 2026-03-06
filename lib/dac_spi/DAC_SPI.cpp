#include "DAC_SPI.hpp"

DAC_SPI::DAC_SPI(int _syncPin, int _ldacPin, int _clrPin) : syncPin(_syncPin), ldacPin(_ldacPin), clrPin(_clrPin) {}

void DAC_SPI::init() {
    initSPIControl();
    initDAC();
}

void DAC_SPI::deinit() {
    // Power down DAC A and B
    sendSPICommand(0b001000000000000000100011); // Command 100 (power down), data=0x0023
    delay(100); // Wait for power down to complete 

    digitalWriteFast(syncPin, LOW);
    digitalWriteFast(ldacPin, LOW);
    digitalWriteFast(clrPin, LOW);
    pinMode(syncPin, INPUT);
    pinMode(ldacPin, INPUT);
    pinMode(clrPin, INPUT);

    SPI.end();

    Debug::msg("DAC A and B powered down");
    Debug::msg("SPI deinitialized.");
}

void DAC_SPI::update(uint16_t A, uint16_t B) {
    digitalWriteFast(ldacPin, LOW);   // Prevent auto-update
    digitalWriteFast(syncPin, LOW);   // Start transaction
    
    SPI.transfer16((0b00000000 << 8) | (A >> 8));        // Cmd A + high byte of A
    SPI.transfer(A & 0xFF);      // Low byte of A
    digitalWriteFast(syncPin, HIGH);
    digitalWriteFast(syncPin, LOW);
    SPI.transfer16((0b00000001 << 8) | (B >> 8));      // Cmd B + high byte of B
    SPI.transfer(B & 0xFF);      // Low byte of B
    
    digitalWriteFast(syncPin, HIGH);  // End transaction
    digitalWriteFast(ldacPin, HIGH);  // Update both DACs simultaneously
}

// Uses SPI.h
void DAC_SPI::initSPIControl() {
    // Initialize SPI with standard library
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV2);  // ~30 MHz clock (clock_div4 is slower)
    SPI.setDataMode(SPI_MODE0);           // CPOL=0, CPHA=0
    SPI.setBitOrder(MSBFIRST);            // Most significant bit first
    
    // Initialize control pins
    pinMode(syncPin, OUTPUT);
    pinMode(ldacPin, OUTPUT);
    pinMode(clrPin, OUTPUT);
    digitalWrite(syncPin, HIGH);
    digitalWrite(ldacPin, HIGH);
    digitalWrite(clrPin, HIGH);
    
    Debug::msg("SPI Control initialized (using SPI.h library)");
}


void DAC_SPI::initDAC() {
    
    initSPIControl();

    // Power down DAC A and B
    sendSPICommand(0b001000000000000000100011); // Command 100 (power down), data=0x0023
    delay(100); // Wait for power down to complete 

    // Power up DAC A and B
    sendSPICommand(0b001000000000000000000011); // Command 100 (power up), data=0x0003
    delay(100); // Wait for power up to complete
    Debug::msg("DAC A and B powered up");

    // Software reset (optional)
    sendSPICommand(0b001010000000000000000001); // Command 101 (reset), data=0x0001
    delay(100); // Wait for reset to complete
    Debug::msg("Software reset complete");

    sendSPICommand(0b001100000000000000000000); // Command 110 (LDAC Asynchronous Mode)
    delay(100); // Wait for LDAC Asynchronous Mode to complete
    Debug::msg("LDAC Asynchronous Mode enabled");

    // Enable internal reference (2.5V) + gain=2 (0-5V range)
    sendSPICommand(0b001110000000000000000001); // Command 111 (enable ref), data=0x0001
    delay(10); // Wait for reference to stabilize
    Debug::msg("Internal reference enabled");

    delay(100);
    uint16_t A = 32768;
    uint16_t B = 32768;

    update(A, B);
    Debug::msg("DAC A and B set to ");
    Debug::msg(A);
    Debug::msg(" & ");
    Debug::msg(B);
}

void DAC_SPI::SPITransfer24(uint32_t frame) {
    SPI.transfer16((frame >> 8) & 0xFFFF);  // Send bits 23-8 (16 bits)
    SPI.transfer(frame & 0xFF);             // Send bits 7-0 (8 bits)
} 
  
void DAC_SPI::sendSPICommand(uint32_t frame) {
    digitalWriteFast(syncPin, LOW);
    SPITransfer24(frame);
    digitalWriteFast(syncPin, HIGH);
}