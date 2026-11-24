# Firmware log viewer

A UI wrapper for the firmware's **existing serial output**. It copies the
logged verdict, match, color, score, control, sample count, calibration, and
channel values. It does not classify samples, apply thresholds, average or
normalize channels, or derive a match from a score. No firmware changes are
needed for this viewer when the device is already producing serial results.

## Open the page

From the repository root:

```sh
python3 -m http.server 8000 --bind 127.0.0.1 --directory visualization
```

Open http://localhost:8000 in Chrome or Edge for live USB access. Pasted and
saved logs also work without a device.

## Use it with the firmware

1. Use a **serial-log build**, such as `dual` or `tcs3448`. This branch builds
   on Isaac's calibration update (`b095c82`) with the five-color bench profile
   described below. The viewer copies the firmware log without analysis.
2. Close any other serial monitor using the board's USB port.
3. Click **Connect USB log**, select the board's USB serial port, and allow access.
   The page opens the same 115200-baud stream as the terminal.
4. Click **Color · c** or **LFA · l**. These send exactly one existing console
   command (`c` or `l`); the firmware still performs the measurement and analysis.
5. Results and the following raw/reflectance lines fill in as the log arrives.
   Expand **Original lines for this result** to compare the display with its source.
6. **Export CSV** before reloading. The history is temporary and stays in this browser.

Opening the port may reset some boards. If the log begins after the startup
`source=hardware` / `source=MOCK` line, source is shown as **not reported**.
The page does not infer whether a reading came from physical sensors.

The existing `*-ble` builds select the BLE sink instead of the serial log sink;
that old 13-byte packet omits color names and spectra. For this log viewer, use
an existing serial-log build. This page neither changes nor extends Bluetooth.
Keep the acquisition and LED settings specified for the active calibration profile.

For offline review, paste plain serial text and click **Display pasted log**,
or open a `.txt` / `.log` file. These inputs are labeled separately from live USB.
Import controls are disabled while USB is connected to avoid mixing sessions.
Files are read locally; nothing is uploaded.

## What is displayed

| Existing log field | UI |
| --- | --- |
| `mode`, `verdict` | Exact firmware test mode and verdict |
| `match`, `color` | Exact labels, including `none` |
| `score`, `control`, `samples` | Printed values, including their original precision |
| `color_match` status, nearest, runner-up and margin | Exact logged decision, candidates and distance lead |
| `color_match` runner-up score and limits | Preserved on the record and CSV |
| `zones`, `metric`, `calibration` | Preserved on the record/CSV; calibration displayed when logged |
| `raw` / `test_raw` | Primary sensor values by logged channel index |
| `control_raw` | Control-zone values by logged channel index |
| `refl` | Firmware reflectance values, unchanged |
| `main: measurement failed: ...` | Original error; no fabricated assay verdict |

A summary appears immediately, then its following spectrum lines attach to
that same result. Missing fields stay absent. In particular, an uncalibrated
LFA log still displays `INVALID`, `0.000`, `0.00`, and `missing_or_invalid`
exactly as printed; the UI does not reinterpret those zeros. Individual colorimetric
results do not contain a calibration ID/date, so the UI does not invent one.
The firmware startup `color_profile` line is visible in the original console log.
Older logs without `color_match` diagnostics display those fields as not reported.

The parser handles split USB chunks, ANSI colors, CRLF/LF, EOF without a final
newline, and unrelated diagnostic lines. A malformed summary, reboot, or
measurement failure stops subsequent spectrum lines attaching to an old result.
Only actual log lines are recognized; photos/OCR or manually hard-wrapped
spectrum lines should be converted back to plain serial text first.

The console shows the latest 2,000 lines. Parsed records and their vectors stay
available for CSV until reload. `received_at` is the browser's receive/import
time, not the experiment date. Device uptime is copied from the log prefix.

## Five-color bench profile

The `bench5-20261123-map1` firmware profile uses measured **white, yellow,
turquoise, orange and deep_purple** references. The white baseline and color
references come from 28 labeled readings collected on November 23, 2026 at
50 mA, 100 ms, 256x gain and 5 samples per result. The withdrawn tape-covered
green readings and earlier yellow positioning readings are excluded.

The firmware matches mean-normalized spectral shape after white correction.
It first chooses the nearest color, then requires a distance no greater than
**0.025** and a lead of at least **0.010** over the next closest color.
These provisional settings replace the earlier yellow-only cutoff trial.
The extra `color_match` diagnostic line explains `matched`, `too_far`, or
`ambiguous` decisions. The browser copies it without applying either rule.
See the [50 mA flash/test instructions](../docs/setup.md#five-color-retest-at-50-ma).
LFA dark captures (`d` in Terminal) use `capture=LFA_DARK` with separate
`dark_test_raw`/`dark_control_raw` lines. They are calibration logs, so the
parser preserves their text without creating or updating assay-history rows.

The `color` field keeps the color name. The `match` field uses Arun's chosen
labels: white → control, yellow → psychedelics, turquoise → fentanyl,
orange → cocaine, and deep_purple → explosives. The viewer displays that
field under **Match (logged)** and includes it in CSV exports. It does not
apply this mapping to old logs or calculate a label in JavaScript.

The `map1` update changes display labels only. In this bench profile, matched
white is `NEGATIVE`, recognized non-white colors are `POSITIVE`, and rejected
readings are `INVALID`. These are configured bench mappings, not validated
substance identifications. Replaying the fitted readings is a regression
check; fresh measurements and repositioning tests are still needed.

White and uniformly gray can share a normalized shape. All 14 channels,
including flicker, remain equally weighted; no dark subtraction was added.
The dual-sensor build at 50 mA uses the separate `paper-lfa-20261123-v1`
LFA profile: left AS7343/control, right TCS3448/test from the operator's view.
See the [LFA setup and final paper-demo check](../docs/dual-sensor-lfa.md#final-paper-demo-check).
Other hardware configurations retain empty LFA references. The wrapper
faithfully displays these outputs and does not correct calibration or
interpretation in JavaScript.

## Verification

A C compiler, Python 3, and Node.js are needed for automated checks:

```sh
python3 visualization/test/generate_logs.py
node visualization/test/log-parser.test.cjs
```

The generator runs synthetic inputs through the **current production**
`strip_analysis.c` and `log_sink.c`. It creates ignored fixtures under
`test/generated/`; these are not physical measurements. CI runs these checks.

Then serve the page and open http://localhost:8000/test/results.html. Browser
checks cover exact displayed values, channel rows, imports, errors, CSV, and
USB stream/command/reconnect behavior using a fake serial port. A real device
is still required to confirm the USB connection and physical readings.

The serial lifecycle follows the [Chrome Web Serial documentation](https://developer.chrome.com/docs/capabilities/serial/).
