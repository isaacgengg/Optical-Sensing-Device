# Sensor notes

## Candidate parts
**TCS3448** (ams OSRAM, tentative). Datasheet DS001121 v1-01, 2025-Feb-06.
14 channels, 380-1000 nm: F1-F8, FZ, FY, FXL, NIR, VIS (clear), FD (flicker).
Driver: `firmware/adapters/tcs3448/`.

## Power and I/O (affects the PCB)
- VDD 1.7-1.98 V (1.98 V absolute max). It cannot run from 3.3 V.
- I2C bus is 1.2 V or 1.8 V, so the ESP32's 3.3 V I/O needs a level shifter.
- GPIO pin selects the I2C voltage ~600 us after power-on: tie to VBUS (1.8 V
  I2C) or GND (1.2 V). It must not float. No I2C for 600 us after power-on.
- INT is open drain, pull-up to 1.8 V.
- An achromatic diffuser over the aperture is required for stable readings.
- The chip has its own LED driver pin (LDR, 4-258 mA). We may not need a
  separate LED GPIO or transistor; not used by the firmware yet.

## I2C address
Fixed at 0x59 (`BOARD_SENSOR_I2C_ADDR`). Up to 1 MHz; we use 400 kHz.

## Key registers
- ID 0x5A = 0x81. Registers 0x58-0x66 need CFG0 (0xBF) bit 4 REG_BANK = 1;
  everything else needs it 0.
- ENABLE 0x80: PON (bit 0), ALS_EN (bit 1).
- CFG1 0xC6 AGAIN[4:0]: 0 = 0.5x, n = 2^(n-1)x, up to 12 = 2048x. Default 9 (256x).
- CFG20 0xD6 auto_smux[6:5] = 3: 18 readings over 3 cycles.
- STATUS2 0x90 bit 6 AVALID = data ready.
- ASTATUS 0x94 then 18 x 16-bit data 0x95-0xB8. Reading ASTATUS latches the
  data; read it all in one burst. ASTATUS bit 7 = saturated.
- 16-bit fields: low byte first, high byte immediately after.

## Timing and integration
t_int = (ATIME + 1) x (ASTEP + 1) x 2.78 us. Full scale = (ATIME + 1) x
(ASTEP + 1), capped at 65535. The driver fixes ASTEP = 599 (1.668 ms/step) and
maps the requested ms to ATIME (1-427 ms, in 1.668 ms steps). One 18-channel read takes about
3 x t_int, so at 100 ms one sample is ~300 ms.

## Calibration
Dark reading, white/reference reading, per-unit calibration, temperature effects.
Autozero (AZ_CONFIG 0xDE) defaults to once before the first measurement.

## Bring-up checklist
- [ ] Address scan (`i2c_bus_scan`) finds the sensor at the expected address.
- [ ] Read the ID register and get the datasheet value.
- [ ] Read real channel data with the LED on and off.
