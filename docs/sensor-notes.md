# Sensor notes

## Candidate parts
Which spectral sensors are we considering, and why? (AS7341 is only a stand-in.)

## I2C address
Fixed or strappable? Record the address and update `BOARD_SENSOR_I2C_ADDR`.

## Key registers
ID, enable/power, gain, integration time, status/data-ready, channel data.

## Timing and integration
Integration time range and step, conversion time, LED settle time needed.

## Calibration
Dark reading, white/reference reading, per-unit calibration, temperature effects.

## Bring-up checklist
- [ ] Address scan (`i2c_bus_scan`) finds the sensor at the expected address.
- [ ] Read the ID register and get the datasheet value.
- [ ] Read real channel data with the LED on and off.
