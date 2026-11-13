# Two-zone LFA firmware

## Physical roles

| Mode | First zone: TCS3448, 0x59 | Second zone: AS7343, 0x39 |
|---|---|---|
| Lateral flow (`l`) | Test line (T) | Control line (C) |
| Colorimetric (`c`) | Color pad | Not configured or sampled by the measurement |

First/second refers to the team's viewing orientation, not a universal strip
flow direction. Verify the actual T/C markings before mounting. Angle, field
of view and placement are mechanical: firmware cannot correct a sensor viewing
both lines or the wrong line. The [photo-layout CAD setup](../hardware/enclosure/README.md)
keeps these roles by reversing the strip entry and aiming each optical path.

Both devices use the existing GPIO21 SDA / GPIO22 SCL bus. No address mux is
needed for 0x59 and 0x39. Verify the specific breakout's supply/level shifting
and combined pull-ups before connecting it; bare chips are not 3.3 V I/O.
The default illuminator is the shared GPIO-controlled light. PR #20 added
`--sensor-leds` to also enable the boards' onboard white LEDs, which the new
optical module uses. Both initialized boards light together, including in
color mode; only TCS is sampled for color. Independently switching zone LEDs
would need a separate firmware change.

## Build and run

```sh
python3 tools/run.py dual PORT
# Or BLE output:
python3 tools/run.py dual-ble PORT
# Onboard LEDs for the photo-layout optical module (default 20 mA):
python3 tools/run.py dual PORT --sensor-leds
```

Omit PORT to build only. The existing `tcs3448` and `tcs3448-ble` builds remain
TCS-only and now reject LFA instead of reading one location twice. `mock` and
`ble` use independent simulated primary and control streams, clearly labeled
at startup. Each build has its own sdkconfig/build directory.

Startup should find both addresses, identify first/test as TCS3448 and
second/control as AS7343, and report no sensor-init errors. A missing or failed
AS7343 init leaves colorimetric available if TCS and the shared bus are healthy.
Reconnect and reboot to retry initialization. A physically stuck shared bus
can affect both devices; software isolation does not fix that wiring fault.

Each LFA request sets independent gain/exposure, lights the rig, waits 50 ms,
and reads T then C five times. Reads are sequential, not simultaneous; at
100 ms integration and three SMUX cycles this takes roughly 3 seconds plus
overhead. Both adapters restore SMUX, gain and exposure before fresh reads,
check data readiness and reject saturated readings. Acquisition failures
turn the light off and publish no new result (a dashboard may still show its
previous result). Low-level I2C errors include the failing address.

## Calibration (required before interpreting LFA verdicts)

Hardware references in `firmware/main/lfa_refs.h` are intentionally empty.
Pressing `l` still prints `test_raw` and `control_raw` but reports
`verdict=INVALID`, `calibration=missing_or_invalid`, and zero scores.
Synthetic references in the mock build are demonstrations only.

1. Fix the sensor positions, viewing angles and illumination in the final rig.
   Start with 100 ms and 256x for each sensor. Lower exposure/gain if saturated.
2. For each zone, collect repeated dark readings with light blocked (cover the
   sensor/optical path; the command still turns the configured lights on). Record
   `test_raw` and `control_raw` separately. Check for light leaks.
3. Collect repeated readings of blank membrane at each line position under
   the same settings. Use membrane/background appropriate to the assay,
   not the colorimetric white-card or red/green paper references.
4. Fill each zone's `dark` and `blank` arrays and counts in `lfa_refs.h`.
   Both drivers return 14 values in this order:
   `F1 F2 FZ F3 F4 F5 FY FXL F6 F7 F8 NIR VIS FD`.
5. Select visible channels using `channel_mask` (bits 0..10 only). Choose bands
   from measured line/background separation; no band is pre-selected for real
   hardware. For example `1u << 4` selects F4, but is not a validated choice.
6. For each selected channel compute `1 - (raw-dark)/(blank-dark)` and average
   those contrasts. Fit each zone's threshold from known positive, negative,
   faint-line and invalid strips, then validate on separate held-out strips.
   Thresholds must be finite, greater than 0 and at most 1. Recalibrate after
   changing angle, light, gain, exposure, sensor or strip positioning.

A blank reference equal to/below dark, mismatched counts, nonfinite data,
selected samples below dark, invalid channel masks or invalid thresholds
produce INVALID. The metric is a starting reflectance-contrast model, not a
validated assay calibration. Raw counts from different sensors are never
compared to each other. Readings brighter than the blank may produce negative
contrast; they are not clipped into a false line.

With valid calibration: missing control => INVALID; for competitive tests,
C+T => NEGATIVE and C only => POSITIVE. Standard format is also supported by
the domain code. The colorimetric calibration and algorithm remain separate;
its existing references remain applicable only at the original optics/settings.

## Outputs and compatibility

Serial LFA results include first/test and second/control raw spectra, contrast
scores, and calibration status. Colorimetric retains `raw`, `refl`, `match` and
`color`. The BLE v1 layout remains 13 bytes: mode, verdict, T/color score,
control score and count. For LFA, sample count means samples per zone.
Uncalibrated LFA uses INVALID and finite zero scores for existing decoders.
BLE v1 does not distinguish missing calibration from missing control or send
spectra; use the serial build for calibration. Old captures used raw-mean
scores, so do not mix their units with the new contrast scores.

## Validation

Host tests cover both register drivers (ID/bank handling, channel mapping,
gain/exposure, reset restoration, timeout, saturation), independent device
state on one bus, role routing, mode switches, missing/failed control sensor,
LED cleanup, independent mock streams and calibrated competitive/standard
rules. CI builds mock, TCS-only, dual serial and dual BLE variants.

Before accepting hardware:
- Change/cover each zone separately and verify only its raw spectrum responds.
- Run `c` with AS disconnected at boot; confirm TCS color still works.
- Run `l` with either device absent/failing; confirm no new valid result.
- Alternate `c`, `l`, `c` and verify gain/exposure and sensor assignments.
- Test stable repeatability, optical crosstalk and faint-line separation.
- Check behavior after sensor brownouts and power-bank idle periods.

The driver follows ams OSRAM AS7343 DS001046 v6-00, sections 9 and 10
([datasheet](https://look.ams-osram.com/m/5f2d27fff9a874d2/original/AS7343-14-Channel-Multi-Spectral-Sensor.pdf)).
It is an independent adapter using the existing TCS implementation pattern;
both use ID 0x81 and the same auto-SMUX output order, so ID alone does not
identify the physical role. Address and wiring determine that assignment.
