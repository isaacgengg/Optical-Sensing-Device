# Two-zone LFA firmware

## Physical roles

| Mode | First zone: TCS3448, 0x59 | Second zone: AS7343, 0x39 |
|---|---|---|
| Lateral flow (`l`) | Test line (T) | Control line (C) |
| Colorimetric (`c`) | Color pad | Not configured or sampled by the measurement |

First/second refers to firmware acquisition and log order. Facing the current
rig, **left is AS7343 / control (C)** and **right is TCS3448 / test (T)**, as
confirmed by the operator. Verify the actual T/C markings before mounting. Angle, field
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
# Measured red-marker paper demo profile (2026-11-23):
python3 tools/run.py dual PORT --sensor-leds --sensor-led-ma 50
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

`firmware/main/lfa_refs.h` contains the provisional `paper-lfa-20261123-v1`
profile for the measured red-marker paper setup: TCS3448 test at 128x,
AS7343 control at 64x, both 100 ms, GPIO illumination plus 50 mA onboard LEDs.
Only that dual-sensor lighting/acquisition configuration enables the measured
references. Other hardware configurations keep empty references: `l` prints
both raw spectra with `verdict=INVALID`, `calibration=missing_or_invalid`,
and zero scores. Mock builds retain their separate synthetic references.

| Zone | Selected channels | Blank reference | Line threshold |
|---|---|---|---|
| Test / TCS3448 | F2, FZ, F4 | Mean of six blank12 captures | 0.228570715 (22.8571%) |
| Control / AS7343 | F3, F5 | Mean of six blank12 captures | 0.018411562 (1.8412%) |

Dark references are the means of six lights-off captures (batch10). Selected
visible dark channels were zero; FD was 3.0 for test and 0.9 for control and
is excluded from scoring. Each threshold is the midpoint of the closest
line-absent and line-present scores across the 34 labeled captures in
batches11-15. A later stationary control-only batch16 had six additional
captures that agreed with the frozen fit. See the replay data and provenance
in `firmware/test/host/data/README.md`.

The control fit has a small margin: the highest absent score was 1.0999%,
the lowest present score was 2.5824%, and the midpoint leaves 0.7413 percentage
points on each side. The user chose to proceed without placement-repeatability
testing. This profile supports the current paper demonstration; reinsertion,
faint-line performance and real assay outcomes are unvalidated. A valid
calibration flag means the references are mathematically usable, not that
physical accuracy has been established.

For a new optical setup or actual assay, collect new references as follows:

1. Fix the sensor positions, viewing angles and illumination in the final rig.
   Start with 100 ms for both sensors, TCS3448 at 128x and AS7343 at 64x.
   Both gains were reduced after saturation at 256x with 50 mA onboard
   LEDs, including TCS3448 saturation over blank paper during a control-only
   capture. Verify blank readings are unsaturated in the new setup and
   collect fresh references before calibrating. Lower the affected
   sensor's gain further if needed.
2. Collect dark readings with the serial **`d`** command. It explicitly switches
   the GPIO light and both onboard LEDs off, then reads both sensors at the
   same LFA gains/exposures and sample count. Leave plain white paper inserted,
   keep the module assembled, and shield the enclosure/strip opening from
   room light with an opaque cover. Wait for each complete capture before
   pressing `d` again. Save five captures, including `dark_test_raw` and
   `dark_control_raw`; these averages retain three decimal places. Check for
   light leaks by changing the room lighting while keeping the cover in place.
   Dark readings need not be exactly zero. If they change with room lighting,
   improve the shielding before using them as dark references.
3. Collect repeated readings of blank membrane at each line position under
   the same settings. Use membrane/background appropriate to the assay,
   not the colorimetric white-card or red/green paper references.
4. Fill each zone's `dark` and `blank` arrays and counts in `lfa_refs.h`.
   Both drivers return 14 values in this order:
   `F1 F2 FZ F3 F4 F5 FY FXL F6 F7 F8 NIR VIS FD`.
5. Select visible channels using `channel_mask` (bits 0..10 only). Choose bands
   from measured line/background separation. The current paper profile uses
   F2/FZ/F4 for test and F3/F5 for control; actual assay channels need their
   own measured selection. For example `1u << 4` selects F4.
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

### Dark-capture output

The startup console prompt must include `d` before using this feature. A
successful capture prints:

```text
capture=LFA_DARK illumination=off samples=5 test_gain=128 test_ms=100 control_gain=64 control_ms=100
dark_test_raw=...
dark_control_raw=...
```

`illumination=off` records successful software off commands; external light
still needs to be blocked. An LED-control or sensor error rejects the capture.
`d` does not run classification, install calibration, or publish a BLE/assay
result. Its lines remain visible in the serial log; the viewer does not add
them to result history. Normal `l`/`c` commands illuminate their next capture
as usual. Both sensors must be available. Mock-build captures are synthetic.

### Paper-line bench capture sequence

This sequence prepares a hand-drawn paper-line demonstration. Use the same
paper, mark width, insertion stop and assembled optics throughout. Keep
TCS3448 at 128x, AS7343 at 64x, both at 100 ms, and illuminated captures at
50 mA. Each key press collects five samples per sensor; send the full output
from each press. Hold the strip still until that capture finishes.

| Order | TCS/test zone | AS/control zone | Key | Captures |
|---|---|---|---|---|
| 1 | White, module externally shielded | White, module externally shielded | `d` | 5 |
| 2 | White, external shield removed | White, external shield removed | `l` | 5 |
| 3 | Red line | White | `l` | 5 |
| 4 | White | Red line | `l` | 5 |
| 5 | Red line | Red line | `l` | 5 |
| 6 | White again | White again | `l` | 3 |

The final blank checks drift after the strip changes. Keep the board running
between batches. Label each batch explicitly. Earlier 256x TCS captures stay
separate; do not divide them by two and call them new measured references.
Earlier 128x/64x batches07-09 had inconsistent control responses and were not
used to fit the installed profile. Fresh batches10-15 supplied dark, both
single-line conditions, both-lines and before/after blanks under one observed
boot. Control-only darkening remained smaller than both-lines darkening;
the threshold uses the weaker control-only condition. Batch16 checks another
stationary control-only series without changing that threshold.

Placement-repeatability validation was explicitly deferred for this paper
demo. Fresh device readings can check the flashed implementation against the
configured competitive logic below, but do not establish placement accuracy.
Real assay strips require their own calibration and independent known-outcome
validation, including reinsertion and faint-line checks.

With valid calibration: missing control => INVALID; for competitive tests,
C+T => NEGATIVE and C only => POSITIVE. Standard format is also supported by
the domain code. The colorimetric calibration and algorithm remain separate;
its existing references remain applicable only at the original optics/settings.

## Outputs and compatibility

Serial LFA results include first/test and second/control raw spectra, contrast
scores, and calibration status. Colorimetric retains `raw`, `refl`, `match` and
`color`. The BLE v1 layout remains 13 bytes: mode, verdict, T/color score,
control score and count. For LFA, sample count means samples per zone.
The calibrated dual build prints `lfa_profile=paper-lfa-20261123-v1` with
`validation=paper_bench_provisional` and both thresholds at startup.
Uncalibrated LFA uses INVALID and finite zero scores for existing decoders.
BLE v1 does not distinguish missing calibration from missing control or send
spectra; use the serial build for calibration. Old captures used raw-mean
scores, so do not mix their units with the new contrast scores.

## Validation

Host tests cover both register drivers (ID/bank handling, channel mapping,
gain/exposure, reset restoration, timeout, saturation), independent device
state on one bus, role routing, mode switches, missing/failed control sensor,
LED cleanup, independent mock streams and calibrated competitive/standard
rules. Measured-profile tests replay the 34 fitting captures and six later
stationary captures through the C classifier. Configuration tests keep the
measured refs disabled with LEDs off, 20 mA, or only one sensor, and verify
that mock mode retains synthetic behavior. CI builds mock, TCS-only, dual
serial and dual BLE variants.

### Final paper-demo check

On November 23, 2026, the operator reported that all four paper patterns
produced the expected results and requested this code as the final demo
version. The left/right orientation below is from the operator's view:

| Paper pattern | Left: AS7343 / C | Right: TCS3448 / T | Expected result | Operator report |
|---|---|---|---|---|
| Control only | Red line | White | POSITIVE | Passed |
| Both lines | Red line | Red line | NEGATIVE | Passed |
| Test only | White | Red line | INVALID | Passed |
| Blank | White | White | INVALID | Passed |

The supplied blank log at uptime 251133 ms reports `calibration=valid`,
`score=0.063`, `control=-0.06`, `samples=5`, and `verdict=INVALID`. Both scores
are below the configured line thresholds. Detailed logs and trial counts for
the other three final patterns were not supplied; their pass status is the
operator's report. The final check does not change the fitted references or
thresholds. Placement repeatability remains outside the requested demo scope,
and these paper checks do not establish real-assay accuracy.

### Additional hardware validation

For broader hardware validation beyond this paper demo:
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
