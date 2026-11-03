// Hamamatsu S13683-02WT I2C color sensor (datasheet KPIC1103E).
// Implements optical_sensor_t over i2c_bus.
//
// Each read() runs one manual-mode measurement (the sensor integrates R, G, B
// and the correction channel one after another, so it takes 4 x the
// integration time) and returns 4 channels, in this order:
//   [0] Blue 460 nm   [1] Green 530 nm   [2] Red 615 nm   [3] Correction
// Values are raw counts. The correction channel sees light without a color
// filter; the datasheet says to subtract it from RGB for accurate color. That
// is analysis, so it is left to the domain.
//
// read() returns ESP_ERR_INVALID_RESPONSE if any channel saturated (65535;
// lower the integration time or gain), ESP_ERR_TIMEOUT if the measurement
// never finished.
//
// HARDWARE: VDD 2.25-3.63 V, so it runs from the ESP32's 3.3 V with no level
// shifter. The datasheet specifies 2.2 kOhm I2C pull-ups; the ESP32's internal
// pull-ups are much weaker.
#pragma once
#include <stdint.h>
#include "optical_sensor.h"
#include "timebase.h"
#include "i2c_bus.h"

#define S13683_CHANNEL_COUNT 4
#define S13683_I2C_ADDR 0x2A  // fixed

// `time` is used to wait for the measurement without busy-waiting.
optical_sensor_t *s13683_create(i2c_bus_handle_t *bus, uint8_t address, timebase_t *time);
