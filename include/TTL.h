#ifndef TTL_H
#define TTL_H

#include <Arduino.h>

#define LASER_ON 7

// Function declaration
void initTTL();

// Fast laser control macros
#define LASER_ON_FAST() GPIO2_DR_CLEAR = (1 << 17)
#define LASER_OFF_FAST() GPIO2_DR_SET = (1 << 17)

#endif