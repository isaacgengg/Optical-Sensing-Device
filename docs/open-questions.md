# Open questions

| Question | Owner | Asked on | Status | Default assumption |
|---|---|---|---|---|
| Which optical sensor part ships? | | | Tentative: TCS3448 | `adapters/tcs3448` driver; mock sensor stays the default build. Needs 1.8 V supply and a level shifter (see `docs/sensor-notes.md`) |
| What channel data does the analysis need? | | | | Up to 16 float channels per reading (`DC_MAX_CHANNELS`); TCS3448 gives 14; the mock uses the same layout |
| Strip types: lateral flow, colorimetric, or both? | | | Answered: both, as separate tests | Two analysis paths in the domain; mode chosen per test at runtime |
| How is the test mode chosen? | | | Tentative: keypad button on the device | Keypad press sets the mode before each test |
| Keypad: which part, how many keys, matrix or individual GPIOs? Is there a display or other feedback for the selected mode? | | | | None; no keypad adapter yet |
| Measurement method: are test and control lines measured separately? | | | Answered: two sensors | First/test line: TCS3448 (0x59); second/control line: AS7343 (0x39). Alternating reads under shared illumination. Optical alignment and per-zone calibration require bench validation. |
| Lateral flow format: standard or competitive? | | | Competitive selected | C+T = negative, C only = positive; absent C = invalid. Standard also supported. |
| Colorimetric: which pad colors map to which results (reference chart or calibration curve)? | | | | Nearest-match against reference reflectance curves in `firmware/main/color_refs.h` (made-up white, clear, purple, orange); replace with measured pads |
| MobileDetect BLE message format, service and characteristic UUIDs? | | | | Placeholder v1 format (`mobiledetect_protocol`: mode, verdict, T and C scores, sample count), random 128-bit UUIDs |
| Does MobileDetect need to trigger measurements from the phone? | | | | No; measurements start from device input (`command_source`, keyboard stand-in for now) |
| Exact ESP32 variant and final pin assignments? | | | Variant answered: ESP32-D0WD-V3 rev v3.1 on a Hosyond ESP-WROOM-32 30-pin devkit. Pins still open | `esp32` target; placeholder pins in `firmware/main/board.h` (the enclosure wiring uses its SDA 21 / SCL 22) |
| LED: does intensity control matter (PWM) or is on/off enough? | | | | On/off via GPIO (`led_gpio`) |
| Repo hosting and IP: should this repo be private, and under which org? | | | | Keep private until decided |
