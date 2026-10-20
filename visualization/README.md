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
- Latest verdict, numeric score, and sample count.
- History table of device and simulated results, newest first.

All data is illustrative. Scores are placeholder metrics, not confidence,
concentration, or validated classification thresholds. The page does not
perform strip analysis or communicate with MobileDetect.

## Relationship to firmware

The display entry point `handleResult(result)` accepts:

```javascript
{ verdict: 0, score: 0.18, sample_count: 8 }
```

Verdict values match `strip_verdict_t` in
`firmware/core/domain/include/strip_types.h`: 0 = negative, 1 = positive,
2 = invalid.

These fields match the current placeholder result payload in
`firmware/adapters/mobiledetect_protocol/`: version (u8), verdict (u8),
score (little-endian float32), sample count (little-endian u16), totaling
8 bytes. The payload does not include test mode, control score, or raw channels.

The page's BLE receiver (`decodeResult`) checks the payload length and version,
decodes the fields, and passes the result to `handleResult`. The service and
characteristic UUIDs must match `firmware/adapters/ble_mobiledetect/`. Because
the payload has no test mode or control score, the page cannot show them yet.

## Quick manual check

1. Serve the folder and open http://localhost:8000; confirm simulation mode and the empty history message.
2. Click the button three times; confirm negative, positive, and invalid results.
3. Check that each result matches the newest history row.
4. Reload; confirm that the latest result and history are reset.
