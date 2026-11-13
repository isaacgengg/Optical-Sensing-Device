# Wiring for the photographed board layout

TCS3448 reads the **test line and color pad**; AS7343 reads **control**.
Both component-side photos show AS above TCS with the USB connector at the
top. Both breakouts have their headers above their sensors and LEDs below.
The orientation changes the physical pin positions, not the required nets.

The old row/column maps, cut lengths and routed-wire picture were for the
opposite board orientation. Do not use them to wire this assembly. The close
photo appears to have a different grid count from the old CAD, and the raised
boards introduce parallax. Actual solder-hole assignments and underside wire
routes are **not established** by these photos. Follow board silkscreen and
verify continuity with power disconnected.

## Connections that remain the same

| Net | Reference wire colour | Connect |
|---|---|---|
| 3V3 | Red | ESP32 3V3 → TCS VIN and AS VIN |
| Ground | Black | ESP32 GND → both sensor GNDs and LCD GND |
| SDA | Orange | ESP32 D21 → both sensor SDAs and LCD SDA |
| SCL | Yellow | ESP32 D22 → both sensor SCLs and LCD SCL |
| LCD supply | White | ESP32 VIN / appropriate 5 V source → LCD VCC |
| Sensor GPIO, INT | — | Unused by this firmware |
| Old external LED | — | GPIO4 drives the old external-light circuit; onboard LEDs use I2C |

These colours are a wiring reference, not confirmation of unseen wire routes.
Sensor breakout addresses stay **TCS3448 0x59**, **AS7343 0x39**. GPIO21/22
and the firmware channel order stay the same. The LCD address is normally
0x27 (some backpacks use 0x3F); check the actual module.

**LCD pull-ups:** if the backpack is powered at 5 V, its SDA/SCL pull-ups must
not pull the ESP32 bus to 5 V. Use the existing verified 3.3 V bus arrangement,
remove the backpack pull-ups as appropriate, or provide level shifting.

## Header orientation

With USB toward the top:

| View | Header order from left to right |
|---|---|
| Component side, as in the photos | INT, GPIO, SDA, SCL, GND, VIN |
| Solder side, CAD +X direction | VIN, GND, SCL, SDA, GPIO, INT |

The model uses vendor pin pitch 2.54 mm, VIN 6.35 mm left of the sensor
centre in the solder-side CAD view. Its **provisional** pin centres are:

| Pin | CAD X (mm) | TCS Y (mm) | AS Y (mm) |
|---|---:|---:|---:|
| VIN | 66.65 | 30.969 | 49.969 |
| GND | 69.19 | 30.969 | 49.969 |
| SCL | 71.73 | 30.969 | 49.969 |
| SDA | 74.27 | 30.969 | 49.969 |
| GPIO | 76.81 | 30.969 | 49.969 |
| INT | 79.35 | 30.969 | 49.969 |

These are derived geometry, **not solder-hole coordinates or measured wire
lengths**. They move with `enclosure/as-built-layout.json`. The updated native
`Wiring.cs` creates coloured pin-net markers rather than claiming to reproduce
the installed wiring. Existing soldered connections need no change if they
already connect the correct labelled pins.

## Bring-up

1. With power disconnected, check the labelled nets and rule out adjacent-pin
   shorts. Confirm sensor-board VIN, GND and header direction independently.
2. Start the dual build and verify both I2C addresses and sensor identities.
3. Use `python3 tools/run.py dual PORT --sensor-leds` for the optical module's
   onboard lighting (default 20 mA). Set a different fixed current with
   `--sensor-led-ma N` if needed and collect calibration at the same setting.
4. `l` samples TCS=test and AS=control. `c` samples TCS only. The existing LED
   helper lights both initialized sensor boards together even in color mode;
   it does not perform independent zone lighting.
5. Cover one viewing zone at a time to verify the physical mapping. Recollect
   dark/blank and color references after the optics change. Mechanical
   rotation does not fix firmware communication or SMUX errors.

See the [CAD setup and change list](enclosure/README.md) and
[dual-sensor firmware setup](../docs/dual-sensor-lfa.md). The updated neutral
models are in `enclosure/exports/as-built/`; root native files and earlier
renders are still the legacy layout until rebuilt in Inventor.
