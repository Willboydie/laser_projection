#ifndef DAC_H
#define DAC_H

#include <Arduino.h>
#include <SPI.h>  // Add SPI library for control functions

#define SYNC 10
#define LDAC 9
#define CLR  8

void SPITransfer24(uint32_t frame);
void sendSPICommand(uint32_t frame);
void initDAC();

void initSPIControl();

void FAST_DAC_UPDATE(uint16_t A, uint16_t B);


#endif