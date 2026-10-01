// Thin wrapper over the ESP-IDF i2c_master API so every I2C transaction goes
// through one place (one timeout, one retry policy, one log format).
// Free of ESP-IDF I2C types so drivers don't need ESP-IDF I2C headers.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

typedef struct i2c_bus i2c_bus_handle_t;      // opaque
typedef struct i2c_bus_dev i2c_bus_dev_t;     // opaque

// Returns NULL on failure. Single bus instance.
i2c_bus_handle_t *i2c_bus_init(int port, int sda, int scl, uint32_t hz);
esp_err_t i2c_bus_add_device(i2c_bus_handle_t *bus, uint8_t addr, i2c_bus_dev_t **dev_out);
esp_err_t i2c_bus_read_regs(i2c_bus_dev_t *dev, uint8_t reg, uint8_t *buf, size_t len);
esp_err_t i2c_bus_write_reg(i2c_bus_dev_t *dev, uint8_t reg, uint8_t value);
// Bring-up helper: logs every 7-bit address that ACKs.
esp_err_t i2c_bus_scan(i2c_bus_handle_t *bus);
