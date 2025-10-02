#ifndef TTL_H
#define TTL_H

#include <Arduino.h>

#define LASER_ON 7

// Function declaration
void initTTL();

// Fast laser control macros
void LASER_ON_FAST();
void LASER_OFF_FAST();

#endif