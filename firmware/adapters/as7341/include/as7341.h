// PLACEHOLDER: sensor part not confirmed. This is a skeleton.
// TODO(team): rename or replace this driver once the sensor part is confirmed.
#pragma once
#include <stdint.h>
#include "optical_sensor.h"
#include "i2c_bus.h"

optical_sensor_t *as7341_create(i2c_bus_handle_t *bus, uint8_t address);
