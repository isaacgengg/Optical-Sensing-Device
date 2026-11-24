# Setup: build, flash, and run

One script, `tools/run.py`, does everything on Linux, macOS and Windows:

```
python3 tools/run.py tcs3448 PORT
```

That builds the firmware, flashes the board on `PORT`, and opens the serial monitor. You don't need ESP-IDF installed:

- The **build** runs in Docker, using Espressif's official ESP-IDF image (`espressif/idf:v5.5.5`, the same one CI uses).
- **Flashing and the monitor** run on your computer with `esptool`, which the script installs into `.venv/` on first use. Docker Desktop on Mac and Windows can't reach USB, so this part can't run inside Docker.

On **Windows**, type `py` instead of `python3` everywhere in this guide.

## 1. Install once

| | Linux | macOS | Windows |
|---|---|---|---|
| Git | your package manager | `xcode-select --install` | [Git for Windows](https://git-scm.com/download/win) |
| Docker | [Docker Engine](https://docs.docker.com/engine/install/), then `sudo usermod -aG docker $USER` and log out and back in | [Docker Desktop](https://www.docker.com/products/docker-desktop/) | [Docker Desktop](https://www.docker.com/products/docker-desktop/) (WSL 2 backend, the default) |
| Python 3.10+ | usually preinstalled; on Ubuntu/Debian also `sudo apt install python3-venv` | `brew install python` or [python.org](https://www.python.org/downloads/) | [python.org](https://www.python.org/downloads/) (tick "Add python.exe to PATH") |
| Terminal | any | Terminal | **PowerShell** |

Docker Desktop must be **running** (whale icon in the menu bar or tray) whenever you build.

**USB-serial driver.** The devkit has a USB-to-serial chip next to its USB port, usually a CP2102 (Silicon Labs) or a CH340 (WCH). Linux has drivers for both. On macOS and Windows, if `tools/run.py ports` finds nothing, install the [CP210x driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers) or the [CH340 driver](https://www.wch-ic.com/downloads/CH341SER_EXE.html), whichever matches your chip.

**Linux only:** give yourself access to serial ports, then log out and back in:
```
sudo usermod -aG dialout $USER
```
(The group is `uucp` on Arch.)

Then get the code:
```
git clone git@github.com:isaacgengg/Optical-Sensing-Device.git
cd Optical-Sensing-Device
```

## 2. Build, flash, monitor

Run these from the repo root (the `Optical-Sensing-Device` folder).

**Find the port.** Plug in the board, then:
```
python3 tools/run.py ports
```
It prints something like `/dev/ttyUSB0` (Linux), `/dev/cu.usbserial-0001` (macOS) or `COM3` (Windows).

**Build, flash and open the monitor:**
```
python3 tools/run.py tcs3448 /dev/ttyUSB0
```
Use your own port. Then, in the monitor:
- press **`l`** (lateral flow) or **`c`** (colorimetric) to take a measurement; the serial console stands in for the device keypad
- press **`d`** for an LFA dark-reference capture with all illumination off; shield the assembled module from room light and follow the [dark-capture instructions](dual-sensor-lfa.md#dark-capture-output)
- press the board's **EN** (or **RST**) button to restart it and see the boot log from the top
- quit with **Ctrl+]**

Everything the script can do:

| Command | Does |
|---|---|
| `python3 tools/run.py dual PORT` | TCS3448 first/test + AS7343 second/control; TCS-only colorimetric |
| `python3 tools/run.py dual-ble PORT` | Same dual-sensor routing, results over Bluetooth |
| `python3 tools/run.py tcs3448 PORT` | TCS3448 only; LFA unavailable |
| `python3 tools/run.py mock PORT` | Same, with the **mock sensor** (replays made-up readings, no sensor needed) |
| `python3 tools/run.py ble PORT` | Mock sensor, results sent over **Bluetooth** to the [dashboard](../visualization/README.md) |
| `python3 tools/run.py tcs3448-ble PORT` | Real sensor, results over Bluetooth |
| `python3 tools/run.py dual PORT --sensor-leds` | Also light the sensor boards' onboard white LEDs with the GPIO LED (any `tcs3448`/`dual` build; builds into `build-<name>-leds`) |
| `python3 tools/run.py dual PORT --sensor-leds --sensor-led-ma 4` | Same, dimmer: white LED current in mA, 4 (dimmest) to 258, default 20 |
| `python3 tools/run.py tcs3448` | Build only (leave off the port) |
| `python3 tools/run.py monitor PORT` | Reopen the monitor without rebuilding or flashing |
| `python3 tools/run.py ports` | List USB serial ports |
| `python3 tools/run.py qemu` | **No board:** run the mock build in the QEMU emulator |
| `python3 tools/run.py test` | **No board:** host tests and the architecture layer check |

- Each build gets its own folder (`firmware/build-tcs3448`, `firmware/build-mock`, …), so switching between them never needs a clean.
- The first build pulls the image (~3 GB) and compiles everything, which takes several minutes (longer on Windows). Later builds only recompile what changed.
- The script prints each command before running it, so you can see what it's doing or rerun a step by hand.

See [dual-sensor LFA](dual-sensor-lfa.md) for two-zone wiring, calibration and bench checks.

### Five-color retest at 50 mA

The `bench5-20261123-map1` profile recognizes **white, yellow, turquoise, orange,
and deep_purple**. It uses the 28 labeled readings collected on November 23:
5 white, 3 yellow, 6 turquoise, 6 orange and 8 deep purple. The five white
readings supply the new white baseline. The withdrawn, tape-covered green
readings and the earlier yellow positioning trial are excluded.

Keep acquisition at **100 ms, 256x gain, 5 samples, 50 mA onboard LEDs**.
The firmware divides by the white baseline, normalizes each spectrum by its
mean, and compares all 14 channels. A match must have an RMS distance of at
most **0.025** and lead the next closest color by at least **0.010**. These
provisional limits replace the earlier yellow 0.09 / other colors 0.04 trial.
The fitted readings are within 0.005 of their own reference; the closest two
reference shapes are about 0.036 apart. Those are fitting checks, not an
independent accuracy measurement.

On Arun's Mac, exit the serial monitor with **Ctrl+]** and disconnect the
browser's USB connection before flashing, then run from the repository root:

```sh
python3.12 tools/run.py dual /dev/cu.usbserial-0001 --sensor-leds --sensor-led-ma 50
```

The script builds, flashes and opens the monitor. Confirm both startup lines:

```text
light=GPIO4 + sensor onboard LEDs @ 50 mA
color_profile=bench5-20261123-map1 colors=white,yellow,turquoise,orange,deep_purple max_distance=0.025 min_margin=0.010
```

For each color, hold the sample still and press `c` five times, waiting for
each result. Then remove and reinsert it for five more readings. Keep the
LED setting, distance and illumination consistent. Save the summary,
`color_match`, `raw` and `refl` lines, noting which readings were repositioned.

- `status=matched`: the nearest color passed both checks.
- `status=too_far`: the nearest reference exceeded its distance limit.
- `status=ambiguous`: two references were too close to choose reliably.

Rejected results keep `match=none color=none`; the separate diagnostic line
still names the nearest and next closest colors and their distance lead.
The `color` field keeps the measured color name. The `match` field uses
Arun's selected display mapping:

| Color | `match` label |
| --- | --- |
| white | control |
| yellow | psychedelics |
| turquoise | fentanyl |
| orange | cocaine |
| deep_purple | explosives |

The `map1` update changes these labels only; the measured references, shape
calculation and limits are unchanged. For this bench profile, `NEGATIVE`
means matched white, `POSITIVE` means a recognized non-white color, and
`INVALID` means no accepted match. These configured labels do not establish
substance identity. For example, yellow now logs
`verdict=POSITIVE match=psychedelics color=yellow`; rejected readings still
log `match=none color=none`.

Refresh the visualization to show the new diagnostic fields. It copies the
firmware's decisions and scores without classifying in the browser. Older
logs still display, with missing diagnostics shown as not reported.

Keep `--sensor-led-ma 50` on subsequent builds: omitting it resets the
LED-enabled build to its 20 mA default. `ESP_ERR_INVALID_RESPONSE` acquisition
failures still need to be resolved separately from color-match cutoffs.

### What a good TCS3448 run looks like

```
i2c_bus: found device at 0x59        <- wiring OK, sensor answers
i2c_bus: scan done, 1 device(s)
                                     <- no "measurement_init failed" line: sensor ID and setup OK
(press c)
log_sink: mode=COLORIMETRIC verdict=... match=... score=... samples=5
log_sink: raw=179, 422, 1649, ...    <- the 14 channels; cover the sensor or shine light and these change
```

TCS3448 wiring on the devkit (from `firmware/main/board.h`): **SDA → GPIO21, SCL → GPIO22**, plus 3V3 and GND. LED → GPIO4.

### QEMU

`qemu` emulates an ESP32 in Docker, the same on every OS. It runs the mock build only (no real sensor, no Bluetooth). Press `l` or `c` once it boots. Quit with **Ctrl+]**, or **Ctrl+T then Ctrl+X** if your terminal doesn't send Ctrl+] (VS Code's often doesn't). Ctrl+C does *not* quit: it is sent to the device as a keypress. If it's stuck, run `docker ps`, then `docker stop <id>`, from another terminal.

## Troubleshooting

| Symptom | Fix |
|---|---|
| `ports` finds nothing | Try another USB cable (many are charge-only), then install the CP210x or CH340 driver (step 1) |
| `Permission denied: '/dev/ttyUSB0'` (Linux) | You're not in `dialout` yet: run the step 1 command, then log out and back in |
| Flashing stuck at `Connecting.....` | Hold the board's **BOOT** button until it starts writing |
| `Could not open port` while flashing | Something else has the port open, often a monitor in another terminal. Close it |
| `docker not found` or `Cannot connect to the Docker daemon` | Start Docker Desktop (Mac and Windows), or run `sudo systemctl start docker` (Linux) |
| `ensurepip is not available` (Ubuntu/Debian) | `sudo apt install python3-venv`, delete the `.venv` folder, run again |
| `python3` or `py` "is not recognized" (Windows) | Reinstall Python with "Add python.exe to PATH" ticked |
| Boot log has no `i2c_bus` lines | You flashed the mock build. Run `tcs3448` |
| `i2c_bus: scan done, 0 device(s)` | Wiring: check SDA/SCL aren't swapped, and the 3V3 and GND connections |
| `measurement failed: ESP_ERR_INVALID_RESPONSE` | Check the preceding saturation warning for the sensor name/address. For LFA, lower that zone's gain in `firmware/main/lfa_refs.h`, rebuild/flash, then collect calibration references at the new setting. Retest defaults are TCS3448 test at 128x and AS7343 control at 64x after saturation at 256x with 50 mA LEDs. |
| `measurement failed: ESP_ERR_TIMEOUT` | Your copy predates the timebase fix (PR #10). Run `git pull` and rebuild |
