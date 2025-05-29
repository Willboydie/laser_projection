

#ifndef SQUARE_H
#define SQUARE_H

#include <Arduino.h>
#include <math.h>

#define NUM_POINTS 3000
#define MIN_DAC 8192
#define MAX_DAC 57342
#define DAC_CENTER 32767
#define GALVO_MAX_ANGLE_RAD (3.1415926 / 6)  // e.g. ±20° in radians (adjust for your galvo's max spec)
#define PROJECTION_DISTANCE (1395 + 70)  // mm (distance to wall — adjust as needed)
#define PROJECTION_SIZE 100       // mm (half-length of square side on wall)

int squareCoords[NUM_POINTS][2];  // 2000 (x, y) integer points

void generateSquare() {
    int pointsPerSide = NUM_POINTS / 4;
    int idx = 0;

    // Helper lambda to convert wall-space position to DAC value
    auto posToDac = [](float pos_mm) -> int {
        float angle = atan(pos_mm / PROJECTION_DISTANCE);  // radians
        float dacVal = DAC_CENTER + (angle / GALVO_MAX_ANGLE_RAD) * (MAX_DAC - DAC_CENTER);
        return constrain((int)dacVal, MIN_DAC, MAX_DAC);
    };

    // Bottom side (left to right)
    for (int i = 0; i < pointsPerSide; i++) {
        float x_mm = -PROJECTION_SIZE + (2.0 * PROJECTION_SIZE * i) / (pointsPerSide - 1);
        squareCoords[idx][0] = posToDac(x_mm);
        squareCoords[idx][1] = posToDac(-PROJECTION_SIZE);
        idx++;
    }

    // Right side (bottom to top)
    for (int i = 0; i < pointsPerSide; i++) {
        float y_mm = -PROJECTION_SIZE + (2.0 * PROJECTION_SIZE * i) / (pointsPerSide - 1);
        squareCoords[idx][0] = posToDac(PROJECTION_SIZE);
        squareCoords[idx][1] = posToDac(y_mm);
        idx++;
    }

    // Top side (right to left)
    for (int i = 0; i < pointsPerSide; i++) {
        float x_mm = PROJECTION_SIZE - (2.0 * PROJECTION_SIZE * i) / (pointsPerSide - 1);
        squareCoords[idx][0] = posToDac(x_mm);
        squareCoords[idx][1] = posToDac(PROJECTION_SIZE);
        idx++;
    }

    // Left side (top to bottom)
    for (int i = 0; i < pointsPerSide; i++) {
        float y_mm = PROJECTION_SIZE - (2.0 * PROJECTION_SIZE * i) / (pointsPerSide - 1);
        squareCoords[idx][0] = posToDac(-PROJECTION_SIZE);
        squareCoords[idx][1] = posToDac(y_mm);
        idx++;
    }
}

#endif