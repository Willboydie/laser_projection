#include <Arduino.h>

int laser_pin = 10;

void optional_setup2() {
    pinMode(laser_pin, OUTPUT); // Set the laser pin as output
    digitalWrite(laser_pin, LOW); // Ensure the laser is off initially
}

void optional_loop2() {

    digitalWrite(laser_pin, HIGH); // Turn on the laser
    delay(70); // Keep it on for 1 second
    digitalWrite(laser_pin, LOW); // Turn off the laser
    delay(70); // Keep it off for 1 second
}