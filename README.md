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
docs/       setup guide, architecture, open questions, sensor notes
hardware/   PCB design files, BOM, fab outputs, datasheets
tools/      check_layers.sh (architecture check)
.github/    CI
```

## Getting started

**[docs/setup.md](docs/setup.md)**: install, build, flash and monitor on Linux, macOS or Windows (Docker plus `esptool`, no ESP-IDF install needed). It also covers running without a board (QEMU) and the host tests.

Quick reference, once set up:

- **Builds:** `tcs3448` (real sensor), `mock` (no sensor, or QEMU), `ble` (results over Bluetooth). Each builds into its own `firmware/build-<name>/` folder.
- **Measure:** in the serial monitor, press `l` (lateral flow) or `c` (colorimetric). Quit with Ctrl+].
- **Target:** ESP-IDF v5.5.5 (v5.3+ needed for the `i2c_master` API), chip `esp32`. TODO(team): confirm the exact ESP32 variant on the PCB.

## Switch sensor and output

The build picks these up from the `sdkconfig.*` files in `firmware/`:

| File | Sets |
|---|---|
| `sdkconfig.defaults` | Always applied: mock sensor, serial-log output |
| `sdkconfig.tcs3448` | Real TCS3448 sensor |
| `sdkconfig.ble` | Results over BLE instead of the log |

To change other options (BLE device name, etc.), run `idf.py -B build-<name> -D SDKCONFIG=build-<name>/sdkconfig menuconfig` (add `-it` to the `docker run`) → **DetectaChem firmware**.

## Host tests and layer check

To run them in Docker on any OS, see [docs/setup.md](docs/setup.md#6-host-tests-no-board-no-esp32). With cmake and gcc/clang installed natively:

```
cmake -S firmware/test/host -B build/host
cmake --build build/host
ctest --test-dir build/host --output-on-failure

./tools/check_layers.sh
```

New host tests go in the folder under `firmware/test/host/` that matches the layer of the code they test (`domain/`, `usecase/`, or `adapters/`).

Unity is fetched with CMake `FetchContent` (tag v2.6.1), so the first
configure needs network access.

## Architecture

See [docs/architecture.md](docs/architecture.md).

## Placeholders

Everything marked `TODO(team)` in the repo (`grep -rn "TODO(team)"`):

| Placeholder | Where |
|---|---|
| Sensor gain (`DEFAULT_AGAIN`, 256x) | `firmware/adapters/tcs3448/tcs3448.c` |
| Pin and bus assignments (I2C port/SDA/SCL/speed, LED GPIO) | `firmware/main/board.h` |
| I2C pull-ups: internal pull-ups enabled until the PCB has its own | `firmware/platform/i2c_bus/i2c_bus.c` |
| Strip analysis algorithm | `firmware/core/domain/strip_analysis.c` |
| Lateral flow thresholds (`t_threshold`, `c_threshold`) | `firmware/main/main.c` |
| Colorimetric white reference, pad colors, match distance (made-up values) | `firmware/main/color_refs.h` |
| Measurement parameters (integration time, LED settle, sample count) | `firmware/main/main.c` |
| Mock sensor data (made-up values) | `firmware/adapters/mock_sensor/mock_data.c` |
| LED intensity control (PWM via LEDC) | `firmware/adapters/led_gpio/led_gpio.c` |
| MobileDetect message format | `firmware/adapters/mobiledetect_protocol/` |
| MobileDetect BLE service and characteristic UUIDs | `firmware/adapters/ble_mobiledetect/ble_mobiledetect.c` |
| ESP32 variant | this README, `idf.py set-target` |

Open questions behind these are tracked in [docs/open-questions.md](docs/open-questions.md).
