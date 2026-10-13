# Visualization draft

A minimal development dashboard for the optical reader. Open `index.html`
directly in a browser. No installation, dependencies, or build step is needed.

Click **Generate sample result** to cycle through simulated negative, positive,
and invalid results. The latest result and history table update together.
History stays in memory and clears when the page reloads.

## Scope

- Simulation-mode label; no device is connected.
- Latest verdict, numeric score, and sample count.
- History table of simulated results, newest first.

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

Actual BLE support is future work. A receiver would need to validate the payload
length and version, decode its fields, and pass the result to `handleResult`.
The final MobileDetect protocol and UUIDs still require confirmation from
DetectaChem; this draft does not establish that contract.

## Quick manual check

1. Open `index.html`; confirm simulation mode and the empty history message.
2. Click the button three times; confirm negative, positive, and invalid results.
3. Check that each result matches the newest history row.
4. Reload; confirm that the latest result and history are reset.
