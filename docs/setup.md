# Setup: build, flash, and run

This works the same way on Linux, macOS and Windows, with no ESP-IDF install:

- **Build** with Docker, using Espressif's official ESP-IDF image (`espressif/idf:v5.5.5`, the same one CI uses).
- **Flash and monitor** from your own computer with `esptool`, a small Python tool. Docker Desktop on Mac and Windows can't reach USB devices, so flashing happens outside Docker on every OS.

Run every command from the **repo root** (the `Optical-Sensing-Device` folder) unless a step says otherwise. Each command is **one line**. If your terminal or chat app wraps it, paste it into a text editor first and check it's still one line.

## 1. Install once

| | Linux | macOS | Windows |
|---|---|---|---|
| Git | your package manager | `xcode-select --install` | [Git for Windows](https://git-scm.com/download/win) |
| Docker | [Docker Engine](https://docs.docker.com/engine/install/), then `sudo usermod -aG docker $USER` and log out and back in | [Docker Desktop](https://www.docker.com/products/docker-desktop/) | [Docker Desktop](https://www.docker.com/products/docker-desktop/) (WSL 2 backend, the default) |
| Python 3.10+ | usually preinstalled | `brew install python` or [python.org](https://www.python.org/downloads/) | [python.org](https://www.python.org/downloads/) (tick "Add python.exe to PATH") |
| Terminal | any | Terminal | **PowerShell** (not cmd.exe) |

Docker Desktop must be **running** (whale icon in the menu bar or tray) before any `docker` command.

**USB-serial driver.** The devkit has a USB-to-serial chip next to its USB port, usually a CP2102 (Silicon Labs) or a CH340 (WCH). Linux has drivers for both. On macOS and Windows, if no port shows up in step 4, install the [CP210x driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers) or the [CH340 driver](https://www.wch-ic.com/downloads/CH341SER_EXE.html), whichever matches your chip.

**Linux only:** give yourself access to serial ports, then log out and back in:
```
sudo usermod -aG dialout $USER
```
(The group is `uucp` on Arch.)

## 2. Get the code and flashing tools (once)

```
git clone git@github.com:isaacgengg/Optical-Sensing-Device.git
cd Optical-Sensing-Device
```

Then make a Python environment for `esptool`, inside the repo (it's git-ignored).

Linux and macOS:
```
python3 -m venv .venv
.venv/bin/pip install esptool
```

Windows (PowerShell):
```
py -m venv .venv
.venv\Scripts\pip install esptool
```

`esptool` also installs `pyserial`, which provides the serial monitor used in step 4.

## 3. Build

There are three builds. Each goes in its own folder under `firmware/`, so you can switch between them without cleaning anything.

| Build | Folder | Use it for |
|---|---|---|
| **tcs3448** | `firmware/build-tcs3448` | A devkit with the real TCS3448 sensor wired up |
| **mock** | `firmware/build-mock` | A devkit with no sensor, or QEMU (it replays made-up readings) |
| **ble** | `firmware/build-ble` | Mock sensor, results sent over Bluetooth to the dashboard |

**tcs3448**

Linux:
```
docker run --rm -u $(id -u):$(id -g) -e HOME=/tmp -v "$PWD/firmware":/project -w /project espressif/idf:v5.5.5 idf.py -B build-tcs3448 -D SDKCONFIG=build-tcs3448/sdkconfig -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.tcs3448" build
```
macOS:
```
docker run --rm -v "$PWD/firmware":/project -w /project espressif/idf:v5.5.5 idf.py -B build-tcs3448 -D SDKCONFIG=build-tcs3448/sdkconfig -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.tcs3448" build
```
Windows (PowerShell):
```
docker run --rm -v "${PWD}/firmware:/project" -w /project espressif/idf:v5.5.5 idf.py -B build-tcs3448 -D SDKCONFIG=build-tcs3448/sdkconfig -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.tcs3448" build
```

**mock** and **ble**: use the same command for your OS, with the end swapped:

| Build | Replace everything after `espressif/idf:v5.5.5` with |
|---|---|
| mock | `idf.py -B build-mock -D SDKCONFIG=build-mock/sdkconfig build` |
| ble | `idf.py -B build-ble -D SDKCONFIG=build-ble/sdkconfig -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.ble" build` |
| tcs3448 + BLE | `idf.py -B build-tcs3448-ble -D SDKCONFIG=build-tcs3448-ble/sdkconfig -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.tcs3448;sdkconfig.ble" build` |

It worked if it ends with `Project build complete.` The first build pulls the image (~3 GB) and compiles everything, which takes several minutes (longer on Windows). Later builds only recompile what changed.

On Linux, `-u $(id -u):$(id -g)` keeps the build files owned by you instead of root. macOS and Windows don't need it.

## 4. Flash and watch the output

Plug in the devkit and find its serial port:

| OS | How to find it | Looks like |
|---|---|---|
| Linux | `ls /dev/ttyUSB* /dev/ttyACM*` | `/dev/ttyUSB0` |
| macOS | `ls /dev/cu.*` (ignore `Bluetooth-Incoming-Port`) | `/dev/cu.usbserial-0001`, `/dev/cu.SLAB_USBtoUART`, `/dev/cu.wchusbserial…` |
| Windows | Device Manager → Ports (COM & LPT) | `COM3` |

Go into the build folder you want to flash. `flash_args` in there lists the files to write:
```
cd firmware/build-tcs3448
```

Flash it, with `PORT` replaced by your port.

Linux and macOS:
```
../../.venv/bin/python -m esptool --chip esp32 -p PORT -b 460800 write_flash "@flash_args"
```
Windows (PowerShell):
```
..\..\.venv\Scripts\python -m esptool --chip esp32 -p PORT -b 460800 write_flash "@flash_args"
```

Then open the serial monitor.

Linux and macOS:
```
../../.venv/bin/python -m serial.tools.miniterm PORT 115200
```
Windows (PowerShell):
```
..\..\.venv\Scripts\python -m serial.tools.miniterm PORT 115200
```

- Press the board's **EN** (or **RST**) button to restart it and see the boot log from the top.
- Press **`l`** (lateral flow) or **`c`** (colorimetric) to take a measurement. The serial console stands in for the device keypad.
- Quit with **Ctrl+]**.
- To build again, go back to the repo root first (`cd ../..`). The monitor holds the port, so quit it before flashing again.

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

## 5. No board? Run it in QEMU

QEMU emulates an ESP32 inside Docker, the same on every OS. It runs the **mock** build only (no real sensor, no Bluetooth). Use your OS's step 3 command, but add **`-it`** after `docker run`, and replace everything after `espressif/idf:v5.5.5` with:
```
idf.py -B build-mock -D SDKCONFIG=build-mock/sdkconfig build qemu monitor
```
Example for macOS:
```
docker run --rm -it -v "$PWD/firmware":/project -w /project espressif/idf:v5.5.5 idf.py -B build-mock -D SDKCONFIG=build-mock/sdkconfig build qemu monitor
```
Press `l` or `c` once it boots. Quit with **Ctrl+]**, or **Ctrl+T then Ctrl+X** if your terminal doesn't send Ctrl+] (VS Code's often doesn't). Ctrl+C does *not* quit: it is sent to the device as a keypress. If it's stuck, run `docker ps`, then `docker stop <id>`, from another terminal.

## 6. Host tests (no board, no ESP32)

These run the core logic and drivers against fakes on your computer, in the same Docker image. Again, use your OS's start of the step 3 command, but mount the **whole repo**: `-v "$PWD":/repo -w /repo` (Windows: `-v "${PWD}:/repo" -w /repo`). Then add:
```
bash -c "bash tools/check_layers.sh && cmake -S firmware/test/host -B build/host-docker && cmake --build build/host-docker && ctest --test-dir build/host-docker --output-on-failure"
```
Example for macOS:
```
docker run --rm -v "$PWD":/repo -w /repo espressif/idf:v5.5.5 bash -c "bash tools/check_layers.sh && cmake -S firmware/test/host -B build/host-docker && cmake --build build/host-docker && ctest --test-dir build/host-docker --output-on-failure"
```
Passing looks like `Layer check passed.` and `100% tests passed`. CI runs the same checks on every push.

## Troubleshooting

| Symptom | Fix |
|---|---|
| No serial port appears | Try another USB cable (many are charge-only), then install the CP210x or CH340 driver (step 1) |
| `Permission denied: '/dev/ttyUSB0'` (Linux) | You're not in `dialout` yet: run the step 1 command, then log out and back in |
| Flashing stuck at `Connecting.....` | Hold the board's **BOOT** button until it starts writing |
| `docker: Cannot connect to the Docker daemon` | Start Docker Desktop (Mac and Windows), or run `sudo systemctl start docker` (Linux) |
| `syntax error near unexpected token` | The command got split across lines. Paste it as one line |
| `.venv\Scripts\pip` "is not recognized" (Windows) | Python wasn't added to PATH. Reinstall Python with the PATH box ticked, or use `py -m venv .venv` again |
| Boot log has no `i2c_bus` lines | You flashed the mock build. Flash `build-tcs3448` |
| `i2c_bus: scan done, 0 device(s)` | Wiring: check SDA/SCL aren't swapped, and the 3V3 and GND connections |
| `measurement failed: ESP_ERR_INVALID_RESPONSE` | The sensor is saturated (too much light at 256x gain). Cover it or dim the light |
| `measurement failed: ESP_ERR_TIMEOUT` | Your copy predates the timebase fix (PR #10). Run `git pull` and rebuild |

To view results over Bluetooth in the dashboard, see [visualization/README.md](../visualization/README.md).
