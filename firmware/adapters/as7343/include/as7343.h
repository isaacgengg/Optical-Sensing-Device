// AS7343, DS001046 v6-00, 7-bit I2C address 0x39.
// 14 raw channels: F1 F2 FZ F3 F4 F5 FY FXL F6 F7 F8 NIR VIS FD.
// One independent instance. Used for LFA second/control line only.
// Bare IC uses 1.8 V; check the breakout's regulator and level shifting.
#pragma once
#include <stdbool.h>
#include "optical_sensor.h"
#include "timebase.h"
#include "i2c_bus.h"
#define AS7343_CHANNEL_COUNT 14
optical_sensor_t *as7343_create(i2c_bus_handle_t *bus, uint8_t address, timebase_t *time);

// Onboard white LED (Adafruit breakout), driven by the chip's LED pin. `ma` is
// clamped to 4-258 mA and rounded down to an even value. init() turns it off.
// ESP_ERR_INVALID_STATE before init().
esp_err_t as7343_set_led(bool on, uint16_t ma);
