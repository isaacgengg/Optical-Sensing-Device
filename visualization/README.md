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

## Use it with Isaac's firmware

1. Use the existing **serial-log build**, such as `dual` or `tcs3448`. Isaac's
   calibration branch is `isaac/recalibrate-colorimetric-refs`, PR #24,
   commit `b095c82`. This UI branch leaves that firmware unchanged.
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
Keep the same sensor/LED settings Isaac used when calibrating the device.

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
| `zones`, `metric`, `calibration` | Preserved on the record/CSV; calibration displayed when logged |
| `raw` / `test_raw` | Primary sensor values by logged channel index |
| `control_raw` | Control-zone values by logged channel index |
| `refl` | Firmware reflectance values, unchanged |
| `main: measurement failed: ...` | Original error; no fabricated assay verdict |

A summary appears immediately, then its following spectrum lines attach to
that same result. Missing fields stay absent. In particular, an uncalibrated
LFA log still displays `INVALID`, `0.000`, `0.00`, and `missing_or_invalid`
exactly as printed; the UI does not reinterpret those zeros. Colorimetric
logs do not contain a calibration ID/date, so the UI does not invent one.

The parser handles split USB chunks, ANSI colors, CRLF/LF, EOF without a final
newline, and unrelated diagnostic lines. A malformed summary, reboot, or
measurement failure stops subsequent spectrum lines attaching to an old result.
Only actual log lines are recognized; photos/OCR or manually hard-wrapped
spectrum lines should be converted back to plain serial text first.

The console shows the latest 2,000 lines. Parsed records and their vectors stay
available for CSV until reload. `received_at` is the browser's receive/import
time, not the experiment date. Device uptime is copied from the log prefix.

## Review of Isaac's update

PR #24 refreshes the white reference and supplies four paper colors: white,
red, green, and yellow. The latest commit removes turquoise/explosives after
lighting drift pushed a re-read outside the matching distance. It matches mean-normalized spectral shape and
uses a **0.04** maximum distance. That analysis is unchanged here.

Current limits in the source/PR: substance names are placeholder test-paper
mappings; white and uniformly gray can share a normalized shape; the reference
set/threshold still need confirmation in stable optics; all 14 channels,
including flicker, remain equally weighted. Hardware LFA zone references are
still empty. The wrapper faithfully displays those outputs and does not try
to correct calibration or interpretation in JavaScript.

## Verification

A C compiler, Python 3, and Node.js are needed for automated checks:

```sh
python3 visualization/test/generate_logs.py
node visualization/test/log-parser.test.cjs
```

The generator runs synthetic inputs through the **actual unchanged**
`strip_analysis.c` and `log_sink.c`. It creates ignored fixtures under
`test/generated/`; these are not physical measurements. CI runs these checks.

Then serve the page and open http://localhost:8000/test/results.html. Browser
checks cover exact displayed values, channel rows, imports, errors, CSV, and
USB stream/command/reconnect behavior using a fake serial port. A real device
is still required to confirm the USB connection and physical readings.

The serial lifecycle follows the [Chrome Web Serial documentation](https://developer.chrome.com/docs/capabilities/serial/).
