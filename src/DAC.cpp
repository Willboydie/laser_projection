#include <DAC.h>
#include <SPI.h>

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

    sendSPICommand(0b001100000000000000000000); // Command 110 (LDAC Asynchronous Mode)
    delay(100); // Wait for LDAC Asynchronous Mode to complete
    Serial.println("LDAC Asynchronous Mode enabled");

    // Enable internal reference (2.5V) + gain=2 (0-5V range)
    sendSPICommand(0b001110000000000000000001); // Command 111 (enable ref), data=0x0001
    delay(10); // Wait for reference to stabilize
    Serial.println("Internal reference enabled");

    delay(100);
    uint16_t A = 32768;
    uint16_t B = 32768;

    FAST_DAC_UPDATE(A, B);
    Serial.print("DAC A and B set to ");
    Serial.print(A);
    Serial.print(" & ");
    Serial.println(B);

}

