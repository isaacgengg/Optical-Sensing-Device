# Firmware architecture

The firmware follows **Clean Architecture**, in the form usually called
**Ports and Adapters** (or **Hexagonal Architecture**). The goal is practical:
most of the code can be written and tested before the sensor or the PCB
exists, and the core logic runs on a laptop with no ESP32 at all.

## The three rings

```
+----------------------------------------------------------+
|  Adapters and platform                                   |
|  main.c, drivers, mock, LED, BLE, i2c_bus, ESP-IDF       |
|   +--------------------------------------------------+   |
|   |  Use case and ports                              |   |
|   |  measurement.c | optical_sensor, illuminator,    |   |
|   |               | timebase, result_sink            |   |
|   |   +------------------------------------------+   |   |
|   |   |  Domain                                  |   |   |
|   |   |  strip_analysis.c, strip_types.h         |   |   |
|   |   +------------------------------------------+   |   |
|   +--------------------------------------------------+   |
+----------------------------------------------------------+
       dependencies that cross a boundary point inward
```

1. **Domain** (`firmware/core/domain`). Rules about test strips: how to
   combine readings and turn them into a result. Pure C. No I/O, no timing,
   no ports. Numbers in, results out.
2. **Use case and ports** (`firmware/core/usecase`, `firmware/core/ports`).
   The device's procedure ("take a measurement and report it") and the
   interfaces it needs. Still pure C, no ESP-IDF calls.
3. **Adapters and platform** (`firmware/adapters`, `firmware/platform`,
   `firmware/main`). Everything that knows about a chip, a protocol, a
   library, or ESP-IDF.

Rings 1 and 2 together are **the core**. The core builds and runs on a laptop.

## The dependency rule

- A dependency that crosses a ring boundary points inward.
- The domain depends on nothing else in the project.
- The use case depends on the domain and the ports.
- Adapters depend on ports (to implement them) and on platform code
  (ESP-IDF, `i2c_bus`, NimBLE).
- **No sideways dependencies between adapters.** The BLE adapter never
  includes a sensor driver; data moves between adapters only through the
  core. The one exception: `ble_mobiledetect` uses the pure codec in
  `mobiledetect_protocol`.
- `main.c` is the **composition root**: the only place that knows every
  layer. It creates the concrete adapters, passes them to the use case, and
  starts the task.

## Ports

A port is a struct of function pointers plus a `void *ctx`. The core owns the
port headers; adapters implement them.

| Port | Implemented by |
|---|---|
| `optical_sensor.h` | `mock_sensor`, `as7341` (placeholder) |
| `illuminator.h` | `led_gpio` |
| `timebase.h` | `freertos_timebase` |
| `result_sink.h` | `log_sink`, `ble_mobiledetect` |
| `command_source.h` | `console_input` (keypad stand-in) |

Ports are named after what they do in *this device* (a sensor, a light, a
clock, a place results go), not with web-app terms like "repository" or
"gateway". `timebase` exists so the use case can wait for the LED to settle
without calling FreeRTOS, which keeps it host-testable.

## Domain vs use case

- The **domain decides**: given these readings, what is the result? Pure
  functions, trivially testable.
- The **use case does**: set integration time, light on, wait, read samples,
  light off, analyze, publish. It sequences I/O through ports and owns
  timing, but contains no strip rules of its own.

## How to add a new sensor driver

1. Create `firmware/adapters/<part>/` with `CMakeLists.txt`
   (`REQUIRES ports i2c_bus`), `include/<part>.h`, `<part>.c`.
2. Implement `optical_sensor_t`; do all I2C through `i2c_bus`.
3. Add the folder to `EXTRA_COMPONENT_DIRS` in `firmware/CMakeLists.txt`.
4. Add a `DC_SENSOR_<PART>` option to the `DC_SENSOR` choice in
   `firmware/main/Kconfig.projbuild`.
5. Add one `#if` branch in `main.c`.

No core changes.

## How to swap the Bluetooth stack

Only `ble_mobiledetect` changes. It is the single file that includes NimBLE
headers. The protocol codec (`mobiledetect_protocol`) and the core are
unaffected.

## How the rules are enforced

- **`REQUIRES` in each component's `CMakeLists.txt`.** ESP-IDF only exposes
  headers of required components, so an illegal include fails to compile.
  `domain`, `ports`, and `usecase` require no ESP-IDF components.
- **The host build** (`firmware/test/host`) compiles the core with plain
  gcc/clang and a stub `esp_err.h`. Anything in the core that reaches for
  ESP-IDF fails to build there.
- **`tools/check_layers.sh`** greps `#include` lines: the core may not include
  RTOS, driver, BLE, NVS, logging, or adapter headers (only `esp_err.h` from
  ESP-IDF), and adapters may not include each other. CI runs it on every push.

## Input: command_source

`command_source` is the one **driving port**: it tells the device when to
measure and in which mode (lateral flow or colorimetric). `main.c` waits on
it, then calls `measurement_run` with that mode's config. For now
`console_input` implements it with keys typed into `idf.py monitor`
(`l` = lateral flow, `c` = colorimetric).

To use the real keypad or GPIO buttons: write an adapter that implements
`command_source_t` (same steps as a sensor driver) and swap it in `main.c`.
No core changes. If MobileDetect ever triggers measurements, the BLE adapter
can implement the same port.
