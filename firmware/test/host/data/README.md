# Recorded sensor data

`five_color_2026_11_23.h` contains 28 user-labeled TCS3448 captures collected
on November 23, 2026: 5 white, 3 yellow, 6 turquoise, 6 orange and 8 deep
purple. Each row includes the source device uptime and the 14 logged raw
channel values. Each capture already averages five samples; the serial log
rounds those averages to integers.

The requested acquisition conditions were 100 ms integration, 256x gain and
50 mA onboard LEDs. The white batch's supplied boot log reported 50 mA and
ELF prefix `dafd9005c`; later batches did not include another boot log.
Sample positions and optical alignment were not independently verified.
The user withdrew green because tape covered the sample. Those readings,
the earlier yellow positioning trial and any unmeasured red sample are not
included in this profile.

The new white baseline is the channel-wise mean of the five white captures.
Each color reference is its mean raw vector divided channel by channel by
that baseline. The production classifier then mean-normalizes the measured
and reference reflectance vectors before computing RMS distance. All 14
channels retain equal weight.

These readings fit `bench5-20261123`; replaying them in `test_color_refs.c`
checks regression and label consistency, not held-out accuracy. The tests
also reject synthetic midpoints between close colors and unmatched inputs.
The provisional distance limit of 0.025 and minimum lead of 0.010 need fresh
physical tests, including removal and reinsertion of each sample.

## Red-marker paper LFA profile

`paper_lfa_2026_11_23.h` contains 40 user-labeled paired captures from the
November 23, 2026 paper demo. These are measured sensor outputs from red
marker and plain white paper, not known-analyte assay outcomes.

| Batch | Pattern | Paired captures | Use |
|---|---|---:|---|
| 11 | Test only | 5 | Fit |
| 12 | Blank | 6 | Blank reference and fit |
| 13 | Control only | 9 | Fit |
| 14 | Both lines | 6 | Fit |
| 15 | Blank repeat | 8 | Baseline return and fit |
| 16 | Control only, stationary | 6 | Check of frozen fit |

Each vector is a logged, rounded average of five samples per zone. Host
tests replay each averaged vector once; the original individual samples are
unavailable. Both arrays use `F1 F2 FZ F3 F4 F5 FY FXL F6 F7 F8 NIR VIS FD`.
The shared transcript reports hardware TCS3448 at 0x59, AS7343 at 0x39,
128x test gain, 64x control gain, 100 ms per sensor and 50 mA onboard LEDs,
with startup ELF prefix `c39579867`. No intervening reset is shown.

`paper-lfa-20261123-v1` uses the six-capture dark10 means and blank12 means.
The test score averages contrast in F2/FZ/F4; control uses F3/F5. NIR, VIS
and FD are excluded. Thresholds are midpoints between maximum absent and
minimum present scores across batches11-15: 0.228570715 for test and
0.018411562 for control. Batch16 was evaluated without refitting. Its user
confirmed that the paper stayed in place. The user then chose to proceed
without placement-repeatability testing; no reinsertion accuracy is claimed.

The fixture stores expected contrasts computed independently from the saved
double-precision fit, so C replay checks numerical transfer as well as all
four line patterns and the configured competitive verdicts. It is regression
coverage and a stationary follow-up check, not general accuracy validation.

Full source logs, per-batch JSON and the frozen fit are in the local workspace
`analysis/2026-11-23-lfa-tests/` alongside the repository. The fixture includes
the source batch, device uptime and frozen-fit SHA256. The authoritative fit
record is `blank12-vs-repeat15-sequence-fit.json`; the follow-up evaluation is
`control16-frozen-fit-evaluation.json`. Older batches07-09 were excluded after
inconsistent control responses and were not counted as successful validation.

After applying the profile, the operator reported all four final paper
patterns passed: control only POSITIVE, both lines NEGATIVE, test only INVALID,
and blank INVALID. Facing the rig, AS7343/control is on the left and
TCS3448/test is on the right. The supplied final blank log reports
`calibration=valid`, test contrast `0.063`, control contrast `-0.06`, and
`verdict=INVALID` at uptime 251133 ms. Detailed logs for the other three final
patterns were not supplied. These final checks are documented in
`docs/dual-sensor-lfa.md`; they were not used to refit or added as invented
captures to the 40-row regression fixture.
