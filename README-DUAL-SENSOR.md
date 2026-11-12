# Dual-sensor setup and daily use

This branch uses **TCS3448 for the first/test line** and **AS7343 for the
second/control line** during lateral-flow measurements. Colorimetric mode
continues to use TCS3448 only.

## Does the system still work the same way?

The operating workflow is the same: connect the ESP32, build and flash with
`tools/run.py`, then press `c` or `l` in the serial monitor. The new build
names are `dual` for serial results and `dual-ble` for Bluetooth results.

| Part of the system | Behavior on this branch |
|---|---|
| Colorimetric (`c`) | TCS only, five readings at 100 ms and 256x; existing color references and matching algorithm retained |
| LFA (`l`) | Five pairs of readings: TCS/test first, AS7343/control second in each pair |
| Light | Same shared light on GPIO4; turns off after acquisition, including read failures |
| Serial controls | Same `c`, `l`, EN/RST, and Ctrl+] controls |
| Bluetooth dashboard | Same connection workflow and 13-byte v1 result layout |
| LFA scores | Independently calibrated line contrast, replacing the old raw-mean scores |
| Missing AS7343 | LFA unavailable; colorimetric remains available if TCS and the shared bus are healthy |

The firmware has passed software tests and builds; operation with both real
sensors has not yet been verified on the rig. Existing color references remain
appropriate only while the TCS position, light and measurement settings match
the setup used to collect them. Moving TCS into a new enclosure can require
new color calibration too.

## 1. Get this branch

From your existing `Optical-Sensing-Device` folder, with your own changes
committed or saved first:

```sh
git fetch origin
git switch arun/dual-sensor-lfa
git pull --ff-only
```

Start Docker Desktop before building. Use the same Python and USB setup as
before; [the general setup guide](docs/setup.md) covers first-time installation.
On Windows, use `py` in place of `python3`.

Pulling this branch does **not** update the ESP32. You must flash it below.

## 2. Connect and position the sensors

Disconnect power before changing wiring. Both sensors share the existing I2C
bus; they have different addresses, so an address multiplexer is not needed.

| Connection | TCS3448 | AS7343 |
|---|---|---|
| SDA | Existing ESP32 GPIO21 bus | Same GPIO21 bus |
| SCL | Existing ESP32 GPIO22 bus | Same GPIO22 bus |
| Ground | Common ground | Common ground |
| I2C address | `0x59` | `0x39` |
| LFA viewing position | First line: **test (T)** | Second line: **control (C)** |

Supply voltage and signal-level connections depend on the exact breakout
boards. Check their regulator, level shifting and pull-ups before connecting
them; do not connect a bare sensor directly to 5 V power or assume its I/O
accepts ESP32 voltage levels. The bus pin assignments above are the firmware
defaults in [board.h](firmware/main/board.h).

First/second means the viewing orientation chosen for this rig. Check the
strip's T/C markings. Each sensor must see its assigned line without including
the other line. Angle and field of view are set mechanically, not in software.
Keep the existing shared illuminator; this branch does not add separate zone
LED control or modify the enclosure model.

## 3. Flash the serial build first

Find the board's port:

```sh
python3 tools/run.py ports
```

Then build, flash and open the monitor:

```sh
python3 tools/run.py dual PORT
```

Replace `PORT` with the actual value printed above, such as
`/dev/cu.usbserial-0001` on a Mac or `COM3` on Windows. Close any other serial
monitor using that port. Omitting `PORT` builds only and does not flash.

Press EN/RST to inspect startup. Expect the bus scan to find both `0x59` and
`0x39`, followed by these source labels:

```text
source=hardware first/test=TCS3448@0x59 color=TCS3448@0x59
second/control=AS7343@0x39
```

Also check for initialization errors: the source labels describe the configured
roles, not proof that initialization succeeded. `source=MOCK` means simulated
readings, not physical sensor readings.

## 4. Check both modes

1. Press `c` with a color sample at TCS. Expect the familiar `raw`, `refl`,
   `match` and `color` output. AS7343 is not sampled for this command.
2. Press `l` with the strip positioned under both sensors. Expect separate
   `test_raw` and `control_raw` spectra. Five pairs take about three seconds
   plus overhead at the initial settings; reads are sequential.
3. Change or cover each viewing zone separately and verify that its assigned
   raw spectrum responds. This checks placement and optical crosstalk.
4. Alternate `c`, `l`, `c` to check both modes in the assembled rig.

At first, LFA reports `INVALID`, `calibration=missing_or_invalid`, and zero
scores even when both sensors work. This is expected: real LFA calibration
values are intentionally empty. Zero scores here do not mean absent lines.

## 5. Calibrate before interpreting LFA results

Follow the [detailed calibration procedure](docs/dual-sensor-lfa.md#calibration-required-before-interpreting-lfa-verdicts).
For each sensor separately, collect dark readings with the optical path
blocked and blank-membrane readings at its line position. Then use labeled
strips to select visible channels and fit separate test/control thresholds.

Enter those measurements in the hardware definitions of `LFA_TEST_REF` and
`LFA_CONTROL_REF` in [lfa_refs.h](firmware/main/lfa_refs.h), along with the
gain/exposure used to collect them. Do not copy the synthetic mock values or
the colorimetric paper references. Rebuild and flash after editing the file.
The firmware does not automatically learn or save calibration from scans.

Validate using separate known positive, negative, faint-line and invalid
strips. For the configured competitive assay: C+T means negative; C only
means positive; missing C means invalid. These rules require valid calibration
and successful acquisition. Repeat calibration after changing the optics,
illumination, sensor settings or strip position.

## 6. Use the existing Bluetooth dashboard

Once serial checks and calibration are complete, flash:

```sh
python3 tools/run.py dual-ble PORT
```

In a second terminal, from the repository root:

```sh
python3 -m http.server 8000 --directory visualization
```

Open `http://localhost:8000` in Chrome or Edge, click **Connect device**, and
select **DetectaChem-T2**. Continue triggering readings with `c` and `l` in the
serial monitor. Connect Bluetooth after the monitor opens, because opening
the monitor can reset the board. See the [dashboard guide](visualization/README.md).

The Bluetooth build sends results over Bluetooth instead of printing raw
spectra. Use `dual` for calibration. The existing decoder accepts the unchanged
packet layout, but new LFA scores have different units from old captures.
This does not establish compatibility with the external MobileDetect app.

## Troubleshooting and returning to TCS-only use

| Symptom | Next step |
|---|---|
| Only `0x59` appears | Check AS7343 power, level shifting and bus wiring; reboot after fixing it |
| AS initialization fails | Fix the second sensor and reboot to retry; `c` can still work if the bus is healthy |
| `INVALID` with both raw spectra | Check calibration status and references before treating this as an assay result |
| Timeout or saturated reading | Check wiring, illumination and per-zone gain/exposure; recalibrate if settings change |
| Dashboard keeps its previous result | Check the serial error log; acquisition failures publish no new result |
| Need colorimetric before AS is ready | Use `c` in the dual build, or flash `python3 tools/run.py tcs3448 PORT` |

On this branch, `tcs3448` and `tcs3448-ble` support TCS-only colorimetric;
they reject LFA because no second physical zone is available. `mock` and `ble`
are simulated-sensor builds. To reopen the monitor without reflashing, use
`python3 tools/run.py monitor PORT`. Quit the monitor with Ctrl+].
