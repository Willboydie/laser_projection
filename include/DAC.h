#ifndef DAC_H
#define DAC_H

#include <Arduino.h>
#include <SPI.h>  // Add SPI library for control functions

#define SYNC 10
#define LDAC 9
#define CLR  8


// LPSPI_TCR bit helpers (Teensy core doesn't provide SDK-style macros)
#define LPSPI_TCR_FRAMESZ(n)   ((n) & 0xFFF)      // bits 0-11: frame size = n
#define LPSPI_TCR_TXMSK        (1u << 15)         // mask transmit
#define LPSPI_TCR_CPHA         (1u << 30)         // clock phase
#define LPSPI_TCR_CPOL         (1u << 31)         // clock polarity

// LPSPI_CCR bit helpers
#define LPSPI_CCR_SCKDIV(n)    ((n) & 0xFF)       // SCK divider (lower = faster)

void fastSPITransfer24(uint32_t frame);
void sendSPICommand(uint32_t frame);
void initDAC();

// Control functions using SPI.h library for debugging
void initSPIControl();
void sendSPICommandControl(uint32_t frame);
void testDACWiring();

// Ultra-fast DAC update macro for 1MHz performance
// This eliminates all function call overhead
// #define FAST_DAC_UPDATE(A, B) do { \
//     digitalWriteFast(SYNC, LOW); \
//     while (!(LPSPI1_SR & LPSPI_SR_TDF)) {} \
//     LPSPI1_TDR = (((0b00011000 << 16) | ((A) & 0xFFFF)) & 0xFFFFFF); \
//     while (!(LPSPI1_SR & LPSPI_SR_TCF)) {} \
//     (void)LPSPI1_RDR; \
//     while (!(LPSPI1_SR & LPSPI_SR_TDF)) {} \
//     LPSPI1_TDR = (((0b00011001 << 16) | ((B) & 0xFFFF)) & 0xFFFFFF); \
//     while (!(LPSPI1_SR & LPSPI_SR_TCF)) {} \
//     (void)LPSPI1_RDR; \
//     digitalWriteFast(SYNC, HIGH); \
// } while(0)

#define FAST_DAC_UPDATE(A, B) do { \
    sendSPICommand((0b00011000 << 16) | ((A) & 0xFFFF)); \
    sendSPICommand((0b00011001 << 16) | ((B) & 0xFFFF)); \
} while(0)


#endif