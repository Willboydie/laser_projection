#ifndef DAC_H
#define DAC_H

#include <Arduino.h>
#include <SPI.h>

#define SYNC 10
#define LDAC 9
#define CLR  8

// Function declarations
void fastSPITransfer(uint8_t data);
void sendSPICommand(uint8_t cmd, uint16_t data);
void initDAC();

// Ultra-fast DAC update macro for 1MHz performance
// This eliminates all function call overhead
#define FAST_DAC_UPDATE(A, B) do { \
    GPIO2_DR_CLEAR = (1 << 20); /* SYNC LOW */ \
    LPSPI1_TDR = 0b00011000; /* DAC A command */ \
    while (!(LPSPI1_SR & LPSPI_SR_TDF)) {} \
    LPSPI1_TDR = highByte(A); \
    while (!(LPSPI1_SR & LPSPI_SR_TDF)) {} \
    LPSPI1_TDR = lowByte(A); \
    while (!(LPSPI1_SR & LPSPI_SR_TDF)) {} \
    LPSPI1_TDR = 0b00011001; /* DAC B command */ \
    while (!(LPSPI1_SR & LPSPI_SR_TDF)) {} \
    LPSPI1_TDR = highByte(B); \
    while (!(LPSPI1_SR & LPSPI_SR_TDF)) {} \
    LPSPI1_TDR = lowByte(B); \
    while (!(LPSPI1_SR & LPSPI_SR_TDF)) {} \
    GPIO2_DR_SET = (1 << 20); /* SYNC HIGH */ \
} while(0)

#endif