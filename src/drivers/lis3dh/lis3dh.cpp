#include "lis3dh.h"
#include "board.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include <stdio.h>

// Private helpers

// Write one byte to a register.
static void write_reg(uint8_t reg, uint8_t data) {
    uint8_t buf[2] = {reg, data};
    i2c_write_blocking(i2c0, ACCEL_I2C_ADDR, buf, 2, false);
}

// Read one or more bytes from a register.
// For multi-byte reads the LIS3DH requires bit 7 of the register address
// to be set — this enables address auto-increment.
static void read_regs(uint8_t reg, uint8_t* buf, int len) {
    if (len > 1) {
        reg |= 0x80;  // set MSB to enable auto-increment
    }
    i2c_write_blocking(i2c0, ACCEL_I2C_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c0, ACCEL_I2C_ADDR, buf, len, false);
}

// LIS3DH implementation

bool LIS3DH::init() {
    // Initialise I2C0 at 400 kHz
    i2c_init(i2c0, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);

    // Read WHO_AM_I to confirm we are talking to the right device.
    // This should return 0x33 for the LIS3DH.
    uint8_t who;
    read_regs(ACCEL_REG_WHO_AM_I, &who, 1);
    if (who != ACCEL_WHO_AM_I_VAL) {
        printf("LIS3DH WHO_AM_I failed: expected 0x%02X, got 0x%02X\n",
               ACCEL_WHO_AM_I_VAL, who);
        return false;
    }
    printf("LIS3DH found OK (WHO_AM_I = 0x%02X)\n", who);

    // CTRL_REG1: 10 Hz sample rate, normal mode, X+Y+Z enabled.
    // Bits: ODR=0010 (10Hz), LPen=0, Zen=1, Yen=1, Xen=1 → 0x27
    write_reg(ACCEL_REG_CTRL1, 0x27);

    // CTRL_REG4: ±2g range, high-resolution mode disabled.
    // Bits: BDU=0, BLE=0, FS=00 (±2g), HR=0, ST=00, SIM=0 → 0x00
    write_reg(ACCEL_REG_CTRL4, 0x00);

    return true;
}

AccelRaw LIS3DH::read_raw() {
    uint8_t buf[6];
    read_regs(ACCEL_REG_OUT_X_L, buf, 6);

    // Each axis is stored as two bytes, left-justified in
    // 16 bits. Right-shift by 6 to recover the 10-bit signed value.
    // (Normal mode = 10-bit; right-shift amount is 16 - 10 = 6)
    AccelRaw raw;
    raw.x = (int16_t)(buf[0] | (buf[1] << 8)) >> 6;
    raw.y = (int16_t)(buf[2] | (buf[3] << 8)) >> 6;
    raw.z = (int16_t)(buf[4] | (buf[5] << 8)) >> 6;
    return raw;
}

AccelG LIS3DH::to_g(AccelRaw raw) {
    // Normal mode, ±2g range: sensitivity = 4 mg/LSB.
    const float sensitivity = 0.004f;
    AccelG g;
    g.x = raw.x * sensitivity;
    g.y = raw.y * sensitivity;
    g.z = raw.z * sensitivity;
    return g;
}