#ifndef DAC_H
#define DAC_H

#include <Arduino.h>
#include <SPI.h>  // Add SPI library for control functions

#define SYNC 10
#define LDAC 9
#define CLR  8

inline void SPITransfer24(uint32_t frame) {
    SPI.transfer16((frame >> 8) & 0xFFFF);  // Send bits 23-8 (16 bits)
    SPI.transfer(frame & 0xFF);             // Send bits 7-0 (8 bits)
}
  
  
inline void sendSPICommand(uint32_t frame) {
    digitalWriteFast(SYNC, LOW);
    SPITransfer24(frame);
    digitalWriteFast(SYNC, HIGH);
}
  
  
inline void FAST_DAC_UPDATE(uint16_t A, uint16_t B) {
    digitalWriteFast(LDAC, LOW);   // Prevent auto-update
    digitalWriteFast(SYNC, LOW);   // Start transaction
    
    SPI.transfer16((0b00000000 << 8) | (A >> 8));        // Cmd A + high byte of A
    SPI.transfer(A & 0xFF);      // Low byte of A
    digitalWriteFast(SYNC, HIGH);
    digitalWriteFast(SYNC, LOW);
    SPI.transfer16((0b00000001 << 8) | (B >> 8));      // Cmd B + high byte of B
    SPI.transfer(B & 0xFF);      // Low byte of B
    
    digitalWriteFast(SYNC, HIGH);  // End transaction
    digitalWriteFast(LDAC, HIGH);  // Update both DACs simultaneously
}


void initDAC();
void initSPIControl();


#endif