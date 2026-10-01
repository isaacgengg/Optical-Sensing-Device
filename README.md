# DetectaChem Team 2: optical sensing device

An ESP32-based reader for lateral flow and colorimetric test strips. It
illuminates the strip with an LED, measures it with an I2C spectral sensor,
turns the readings into a result, and reports it over Bluetooth Low Energy to
DetectaChem's MobileDetect phone app. The firmware is layered so the core
logic runs and is tested on a laptop, and the full pipeline runs on a devkit
with a mock sensor before the real sensor and PCB exist.

## Layout

```
firmware/   ESP-IDF project: core (domain, ports, use case), adapters, platform, host tests
docs/       architecture, open questions, sensor notes
hardware/   PCB design files, BOM, fab outputs, datasheets
tools/      check_layers.sh (architecture check)
.github/    CI
```

## Prerequisites

- **ESP-IDF v5.5.5** (not installed on the machine that created this repo;
  pinned to the latest stable v5.x at the time, same as CI). v5.3+ is
  required for the `i2c_master` / `esp_driver_*` APIs.
- **Target chip:** `esp32`. TODO(team): confirm the exact ESP32 variant on the PCB.
- **CMake** 3.16+ and gcc or clang for host tests.

## Build and flash

```
cd firmware && idf.py set-target esp32 && idf.py build flash monitor
```

Or without a local ESP-IDF install:

```
docker run --rm -v $PWD/firmware:/project -w /project espressif/idf:v5.5.5 idf.py set-target esp32 build
```

## Switch sensor and output

`idf.py menuconfig` → **DetectaChem firmware**:

- *Optical sensor*: Mock (default) or AS7341 (placeholder skeleton)
- *Result output*: Serial log (default) or BLE
- BLE device name, measurement interval

To build with BLE without menuconfig:
`idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.ble" build`
(delete `firmware/sdkconfig` first if one exists).

## Host tests and layer check

```
cmake -S firmware/test/host -B build/host
cmake --build build/host
ctest --test-dir build/host --output-on-failure

./tools/check_layers.sh
```

Unity is fetched with CMake `FetchContent` (tag v2.6.1), so the first
configure needs network access.

## Architecture

See [docs/architecture.md](docs/architecture.md).

## Placeholders

Everything marked `TODO(team)` in the repo (`grep -rn "TODO(team)"`):

| Placeholder | Where |
|---|---|
| Sensor part (AS7341 is a stand-in): rename/replace driver, fill registers, ID, gain/integration encodings, channel count from datasheet | `firmware/adapters/as7341/` |
| Sensor I2C address | `firmware/main/board.h` (`BOARD_SENSOR_I2C_ADDR`) |
| Pin and bus assignments (I2C port/SDA/SCL/speed, LED GPIO) | `firmware/main/board.h` |
| I2C pull-ups: internal pull-ups enabled until the PCB has its own | `firmware/platform/i2c_bus/i2c_bus.c` |
| Strip analysis algorithm | `firmware/core/domain/strip_analysis.c` |
| Analysis thresholds (`positive_threshold`, `min_signal`) | `firmware/core/domain/include/strip_types.h`, `firmware/main/main.c` |
| Measurement parameters (integration time, LED settle, sample count) | `firmware/main/main.c` |
| Mock sensor data (made-up values) | `firmware/adapters/mock_sensor/mock_data.c` |
| LED intensity control (PWM via LEDC) | `firmware/adapters/led_gpio/led_gpio.c` |
| MobileDetect message format | `firmware/adapters/mobiledetect_protocol/` |
| MobileDetect BLE service and characteristic UUIDs | `firmware/adapters/ble_mobiledetect/ble_mobiledetect.c` |
| ESP32 variant | this README, `idf.py set-target` |

Open questions behind these are tracked in [docs/open-questions.md](docs/open-questions.md).
