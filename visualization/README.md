# Visualization draft

A minimal development dashboard for the optical reader. It shows live results
from the ESP32 over Bluetooth, or simulated ones. No installation,
dependencies, or build step is needed.

```
cd visualization && python3 -m http.server 8000
```

Then open http://localhost:8000 in **Chrome or Edge** (Firefox and Safari have
no Web Bluetooth). Bluetooth only works on https or localhost pages, so serve
the folder rather than opening the file directly.

**Linux only:** Web Bluetooth is off by default. Enable
`chrome://flags/#enable-experimental-web-platform-features` and relaunch Chrome.

Click **Generate sample result** to cycle through simulated negative, positive,
and invalid results. The latest result and history table update together.
History stays in memory and clears when the page reloads.

## Live results over Bluetooth

1. Flash the BLE build (from `firmware/`, delete `sdkconfig` first if present):
   `idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.ble" build flash`
2. Click **Connect device**, pick **DetectaChem-T2**, and pair.
3. Trigger measurements with `l` or `c` in `idf.py monitor`. Each result is
   added to the history marked **Device**.

Opening the serial monitor resets the board and drops the connection; click
**Connect device** again afterwards. The board keeps its pairing keys in flash,
so reconnecting works after a reset. If it still fails with an authentication
error, remove the device from your computer's Bluetooth settings and pair again.

## Scope

- Connection status: simulation mode, connected, or disconnected.
- Latest test type, verdict, numeric score, C-line score, and sample count.
- History table of device and simulated results, newest first.
- Lateral-flow scores show the T-line and C-line signals. Colorimetric results
  show the score and **N/A** for C line.

All data is illustrative. Scores are placeholder metrics, not confidence,
concentration, or validated classification thresholds. The page does not
perform strip analysis or communicate with MobileDetect.

## Relationship to firmware

The display entry point `handleResult(result)` accepts:

```javascript
{ mode: 0, verdict: 0, score: 0.18, control_score: 0.85, sample_count: 8 }
```

Verdict values match `strip_verdict_t` in
`firmware/core/domain/include/strip_types.h`: 0 = negative, 1 = positive,
2 = invalid.

Mode values match `strip_mode_t`: 0 = lateral flow, 1 = colorimetric.
Both scores must be finite numbers; mode and verdict must be recognized
integers, and sample count must be an integer from 0 to 65535.

The current placeholder v1 payload in `firmware/adapters/mobiledetect_protocol/`
is 13 bytes, with multibyte fields encoded little-endian:

| Offset | Field | Type |
| --- | --- | --- |
| 0 | Version (`1`) | u8 |
| 1 | Mode | u8 |
| 2 | Verdict | u8 |
| 3–6 | Score (T-line signal for lateral flow) | float32 |
| 7–10 | Control score (C-line signal; zero for colorimetric) | float32 |
| 11–12 | Sample count | u16 |

The page's BLE receiver (`decodeResult`) checks the payload length and version,
decodes the fields, and passes the result to `handleResult`. The service and
characteristic UUIDs must match `firmware/adapters/ble_mobiledetect/`.
Older v0 messages and malformed results are rejected without changing the
latest result or history.

## Decoder regression checks

With the server running, open http://localhost:8000/test/results.html and
confirm all checks pass. No device or test dependencies are needed. These checks
exercise the actual page in an iframe with the real board captures from
[PR #7](https://github.com/isaacgengg/Optical-Sensing-Device/pull/7):

- Lateral flow: `01 00 00 66 b6 90 44 66 b6 90 44 05 00`
- Colorimetric: `01 01 00 66 b6 90 44 00 00 00 00 05 00`

Both decode to negative, score approximately 1157.7, and 5 samples.
The lateral-flow C-line score is approximately 1157.7; the colorimetric
control score is zero and displays as N/A.

## Quick manual check

1. Serve the folder and open http://localhost:8000; confirm simulation mode and the empty history message.
2. Click the button three times; confirm negative, positive, and invalid results.
3. Check that each result matches the newest history row, including test type
   and C line. The colorimetric sample should show N/A for C line.
4. Reload; confirm that the latest result and history are reset.
