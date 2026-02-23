#pragma once

#include "Constants.hpp"
#include "SPI.hpp"
#include "TTL.hpp"

class Projector {
private:
    uint8_t* pathData;
    uint32_t pointsProjected = 0;

public:
    bool update();
    uint32_t getPointsProjected() const { return pointsProjected };
}
