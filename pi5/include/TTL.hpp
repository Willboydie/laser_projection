#pragma once


class TTL {
private:
    bool laserState = false;

public:
    bool init();
    void updateLaser(uint8_t laser_on) {
        if (laser_on == laserState) return;
        if (laser_on) turnLaserOn();
        else turnLaserOff();
        laserState = laser_on;
    }
    
private:
    // MMIO functions which toggle the laser state GPIO pin HIGH and LOW
    void turnLaserOn();
    void turnLaserOff();
}