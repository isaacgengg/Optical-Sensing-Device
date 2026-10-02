# Open questions

| Question | Owner | Asked on | Status | Default assumption |
|---|---|---|---|---|
| Which optical sensor part ships? | | | | AS7341 stand-in skeleton (`adapters/as7341`); mock sensor until confirmed |
| What channel data does the analysis need? | | | | Up to 16 float channels per reading (`DC_MAX_CHANNELS`); mock uses 8 |
| Strip types: lateral flow, colorimetric, or both? | | | Answered: both, as separate tests | Two analysis paths in the domain; mode chosen per test at runtime |
| How is the test mode chosen? | | | Tentative: keypad button on the device | Keypad press sets the mode before each test |
| Keypad: which part, how many keys, matrix or individual GPIOs? Is there a display or other feedback for the selected mode? | | | | None; no keypad adapter yet |
| Measurement method: are test and control lines measured separately? | | | | No; one reading of the whole window, placeholder threshold on mean signal. Lateral flow needs separate T and C readings, so the optics must separate them |
| Lateral flow format: standard (T line = positive) or competitive (T line = negative)? | | | | Standard |
| Colorimetric: which pad colors map to which results (reference chart or calibration curve)? | | | | None; needs data from DetectaChem |
| MobileDetect BLE message format, service and characteristic UUIDs? | | | | Placeholder v0 format (`mobiledetect_protocol`), random 128-bit UUIDs |
| Does MobileDetect need to trigger measurements from the phone? | | | | No; measurements start from device input (`command_source`, keyboard stand-in for now) |
| Exact ESP32 variant and final pin assignments? | | | | Plain `esp32` target; placeholder pins in `firmware/main/board.h` |
| LED: does intensity control matter (PWM) or is on/off enough? | | | | On/off via GPIO (`led_gpio`) |
| Repo hosting and IP: should this repo be private, and under which org? | | | | Keep private until decided |
