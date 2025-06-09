#ifndef CARTESIAN_SHAPES_CPP
#define CARTESIAN_SHAPES_CPP

#include <math.h>
#include <stdint.h> // for uint16_t

#define MAX_POINTS 20000 // Maximum number of points to generate
#define MAX_ANGLE 0.376 // Maximum angle in radians (e.g., ±90°)

// Struct for a 3D point
struct Point {
    float x;
    float y;
    float z;
};

// Struct for spherical coordinates
struct SphericalCoord {
    float r;
    float theta;
    float phi;
};

// Function to generate circle points
uint16_t cartesianCircle(float c, float r, float inc, Point coords[MAX_POINTS]) {
    uint16_t idx = 0;

    if (2 * M_PI * r / inc > MAX_POINTS) {
        return MAX_POINTS + 1; // Flag too many points
    }

    for (float angle = 0.0f; angle < 2.0f * M_PI; angle += inc) {
        if (idx >= MAX_POINTS) break;
        coords[idx++] = {c, r * cosf(angle), r * sinf(angle)};
    }
    return idx;
}

// Function to generate rectangle points
uint16_t cartesianRectangle(float c, float h, float w, float inc, Point coords[MAX_POINTS]) {
    uint16_t idx = 0;

    if (2 * (h + w) / inc > MAX_POINTS) {
        return MAX_POINTS + 1; // Flag too many points
    }

    for (float y = -w / 2.0f; y < w / 2.0f; y += inc) {
        if (idx >= MAX_POINTS) break;
        coords[idx++] = {c, y, h / 2.0f};
    }

    for (float z = h / 2.0f; z > -h / 2.0f; z -= inc) {
        if (idx >= MAX_POINTS) break;
        coords[idx++] = {c, w / 2.0f, z};
    }

    for (float y = w / 2.0f; y > -w / 2.0f; y -= inc) {
        if (idx >= MAX_POINTS) break;
        coords[idx++] = {c, y, -h / 2.0f};
    }

    for (float z = -h / 2.0f; z < h / 2.0f; z += inc) {
        if (idx >= MAX_POINTS) break;
        coords[idx++] = {c, -w / 2.0f, z};
    }

    return idx;
}

// Helper function to interpolate between two points
static void interpolate(float c, Point start, Point end, float inc, Point coords[MAX_POINTS], uint16_t &idx) {
    float y_start = start.y;
    float z_start = start.z;
    float y_end = end.y;
    float z_end = end.z;
    float dy = y_end - y_start;
    float dz = z_end - z_start;

    float length = hypotf(dy, dz);
    int num_points = (length > 0.0f) ? int(ceilf(length / inc)) : 1;

    if (num_points < 2) num_points = 2;

    for (int i = 0; i < num_points; ++i) {
        if (idx >= MAX_POINTS) break;
        float t = float(i) / float(num_points - 1);
        coords[idx++] = {c, y_start + t * dy, z_start + t * dz};
    }
}

// Function to generate triangle points
uint16_t cartesianTriangle(float c, Point p1, Point p2, Point p3, float inc, Point coords[MAX_POINTS]) {
    uint16_t idx = 0;

    interpolate(c, p1, p2, inc, coords, idx);
    interpolate(c, p2, p3, inc, coords, idx);
    interpolate(c, p3, p1, inc, coords, idx);

    if (idx >= MAX_POINTS) {
        return MAX_POINTS + 1; // Flag too many points
    } else {
        return idx;
    }
}

// Function to convert Cartesian to Spherical
SphericalCoord toSpherical(float c, float y, float z) {
    SphericalCoord sph;

    sph.theta = y / c - (1.0f / 3.0f) * powf(y / c, 3);
    sph.phi = z / c - (1.0f / 3.0f) * powf(z / c, 3) - (z / (2 * c)) * powf(sph.theta, 2);

    // sph.theta = atan2f(y, c);
    // sph.phi = atan2f(z*cos(sph.theta), c);

    return sph;
}

uint16_t toDacCmd(float angle) {
    return constrain((int)((angle / MAX_ANGLE) * 26213 + 32767), 6554, 58980); // Convert angle to DAC command
}

#endif // CARTESIAN_SHAPES_CPP