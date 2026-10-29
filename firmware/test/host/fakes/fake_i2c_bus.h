// Host fake of i2c_bus.h: one device backed by a 256-byte register file.
// Models the TCS3448 register bank: 0x20-0x7F read as 0xEE unless
// CFG0 (0xBF) bit 4 is set.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include "i2c_bus.h"

#define FAKE_I2C_MAX_WRITES 64

typedef struct {
    uint8_t regs[256];
    struct { uint8_t reg, value; } writes[FAKE_I2C_MAX_WRITES];
    size_t write_count;
    size_t burst_count;  // write_regs calls with len > 1
} fake_i2c_state_t;

// Resets all state. Returns a non-NULL bus handle.
i2c_bus_handle_t *fake_i2c_bus_create(void);
fake_i2c_state_t *fake_i2c_state(void);
