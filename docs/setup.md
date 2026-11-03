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
- press the board's **EN** (or **RST**) button to restart it and see the boot log from the top
- quit with **Ctrl+]**

Everything the script can do:

| Command | Does |
|---|---|
| `python3 tools/run.py tcs3448 PORT` | Build, flash and monitor with the **real TCS3448 sensor** |
| `python3 tools/run.py mock PORT` | Same, with the **mock sensor** (replays made-up readings, no sensor needed) |
| `python3 tools/run.py ble PORT` | Mock sensor, results sent over **Bluetooth** to the [dashboard](../visualization/README.md) |
| `python3 tools/run.py tcs3448-ble PORT` | Real sensor, results over Bluetooth |
| `python3 tools/run.py tcs3448` | Build only (leave off the port) |
| `python3 tools/run.py monitor PORT` | Reopen the monitor without rebuilding or flashing |
| `python3 tools/run.py ports` | List USB serial ports |
| `python3 tools/run.py qemu` | **No board:** run the mock build in the QEMU emulator |
| `python3 tools/run.py test` | **No board:** host tests and the architecture layer check |

- Each build gets its own folder (`firmware/build-tcs3448`, `firmware/build-mock`, …), so switching between them never needs a clean.
- The first build pulls the image (~3 GB) and compiles everything, which takes several minutes (longer on Windows). Later builds only recompile what changed.
- The script prints each command before running it, so you can see what it's doing or rerun a step by hand.

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
| `measurement failed: ESP_ERR_INVALID_RESPONSE` | The sensor is saturated (too much light at 256x gain). Cover it or dim the light |
| `measurement failed: ESP_ERR_TIMEOUT` | Your copy predates the timebase fix (PR #10). Run `git pull` and rebuild |
