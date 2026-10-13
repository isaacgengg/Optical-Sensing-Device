// ams OSRAM TCS3448 14-channel spectral sensor (datasheet DS001121 v1-01).
// Implements optical_sensor_t over i2c_bus.
//
// Each read() runs one 18-channel auto-SMUX measurement (3 integration
// cycles) and returns 14 channels, in this order:
//   [0] F1  407 nm   [1] F2  424 nm   [2] FZ  450 nm   [3] F3  473 nm
//   [4] F4  516 nm   [5] F5  546 nm   [6] FY  560 nm   [7] FXL 596 nm
//   [8] F6  636 nm   [9] F7  687 nm   [10] F8 748 nm   [11] NIR 855 nm
//   [12] VIS (clear) [13] FD (flicker photodiode)
// Values are raw ADC counts.
//
// read() returns ESP_ERR_INVALID_RESPONSE if any channel saturated (lower the
// integration time or gain), ESP_ERR_TIMEOUT if data never became ready.
//
// HARDWARE: VDD and the I2C bus are 1.8 V max (1.98 V absolute). The ESP32's
// 3.3 V I/O needs a level shifter. GPIO must not float: tie it to VBUS for
// 1.8 V I2C or to GND for 1.2 V.
#pragma once
#include <stdint.h>
#include "optical_sensor.h"
#include "timebase.h"
#include "i2c_bus.h"

#define TCS3448_CHANNEL_COUNT 14  // I2C address is fixed at 0x59 (BOARD_SENSOR_I2C_ADDR)

// `time` is used to poll for data ready without busy-waiting.
optical_sensor_t *tcs3448_create(i2c_bus_handle_t *bus, uint8_t address, timebase_t *time);
