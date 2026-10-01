# Open questions

| Question | Owner | Asked on | Status | Default assumption |
|---|---|---|---|---|
| Which optical sensor part ships? | | | | AS7341 stand-in skeleton (`adapters/as7341`); mock sensor until confirmed |
| What channel data does the analysis need? | | | | Up to 16 float channels per reading (`DC_MAX_CHANNELS`); mock uses 8 |
| Measurement method: are test and control lines measured separately? | | | | No; one reading of the whole window, placeholder threshold on mean signal |
| MobileDetect BLE message format, service and characteristic UUIDs? | | | | Placeholder v0 format (`mobiledetect_protocol`), random 128-bit UUIDs |
| Does MobileDetect need to trigger measurements from the phone? | | | | No; device measures on a fixed interval (`DC_MEASURE_INTERVAL_MS`) |
| Exact ESP32 variant and final pin assignments? | | | | Plain `esp32` target; placeholder pins in `firmware/main/board.h` |
| LED: does intensity control matter (PWM) or is on/off enough? | | | | On/off via GPIO (`led_gpio`) |
| Repo hosting and IP: should this repo be private, and under which org? | | | | Keep private until decided |
