#include <SPI.h>
#include <Arduino.h>

int slaveSelect = 10;

int betweenBits = 500; // Delay between bits
int between24bits = 1000; // Delay between 24-bit values

void opt_setup() {
    Serial.begin(115200); // Initialize serial communication
    while (!Serial);

    Serial.println("Starting SPI Test...");
    pinMode(slaveSelect, OUTPUT);
    SPI.begin();
    SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
}



void opt_loop() {
    for (uint32_t i = (16000UL << 4); i < (1UL << 24); i++) {  // Loop through 0 to (2^24 - 1)
        
        // Print full 24-bit representation, padded with leading zeros
        for (int bit = 23; bit >= 0; bit--) {
            Serial.print((i >> bit) & 1);
        }
        Serial.println();
        
        digitalWrite(slaveSelect, LOW);  // Enable SPI device
        // Send the 24-bit value in three 8-bit sections
        SPI.transfer((i >> 16) & 0xFF);  // Send most significant byte (MSB)
        digitalWrite(slaveSelect, HIGH);  // Disable SPI device
        delay(betweenBits);  // Delay for visibility
        
        digitalWrite(slaveSelect, LOW);  // Enable SPI device
        SPI.transfer((i >> 8) & 0xFF);   // Send middle byte
        digitalWrite(slaveSelect, HIGH);  // Disable SPI device
        delay(betweenBits);  // Delay for visibility

        digitalWrite(slaveSelect, LOW);  // Enable SPI device
        SPI.transfer(i & 0xFF);          // Send least significant byte (LSB)
        digitalWrite(slaveSelect, HIGH);  // Disable SPI device
        delay(betweenBits);  // Delay for visibility

        digitalWrite(slaveSelect, LOW);  // Enable SPI device
        SPI.transfer(0);          // Turn off LEDs in between 24 bit messages
        digitalWrite(slaveSelect, HIGH);  // Disable SPI device
        delay(between24bits);  // Delay for visibility
    }
}