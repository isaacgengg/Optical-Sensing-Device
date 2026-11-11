// AS7343, DS001046 v6-00, 7-bit I2C address 0x39.
// 14 raw channels: F1 F2 FZ F3 F4 F5 FY FXL F6 F7 F8 NIR VIS FD.
// One independent instance. Used for LFA second/control line only.
// Bare IC uses 1.8 V; check the breakout's regulator and level shifting.
#pragma once
#include "optical_sensor.h"
#include "timebase.h"
#include "i2c_bus.h"
#define AS7343_CHANNEL_COUNT 14
optical_sensor_t *as7343_create(i2c_bus_handle_t *bus, uint8_t address, timebase_t *time);
