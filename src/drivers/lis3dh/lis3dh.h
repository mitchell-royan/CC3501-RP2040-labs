#pragma once
#include <stdint.h>

// Raw acceleration readings (10-bit signed integers)
struct AccelRaw {
    int16_t x, y, z;
};

// Acceleration converted to g-forces
struct AccelG {
    float x, y, z;
};

class LIS3DH {
public:
    // Initialise I2C and configure the accelerometer.
    // Returns false if the WHO_AM_I check fails.
    bool init();

    // Read raw integer values from the sensor.
    AccelRaw read_raw();

    // Convert raw values to g-forces.
    AccelG to_g(AccelRaw raw);
};