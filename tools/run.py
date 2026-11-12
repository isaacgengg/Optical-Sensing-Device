#!/usr/bin/env python3
"""Build, flash and monitor the firmware with one command, on Linux, macOS or Windows.

    python3 tools/run.py tcs3448 PORT   build, flash, then open the serial monitor
    python3 tools/run.py tcs3448        build only
    python3 tools/run.py monitor PORT   reopen the serial monitor (no build, no flash)
    python3 tools/run.py ports          list serial ports, to find PORT
    python3 tools/run.py qemu           run the mock build in the QEMU emulator (no board)
    python3 tools/run.py test           host tests and layer check

Builds: mock, tcs3448, dual, ble, tcs3448-ble, dual-ble. On Windows use `py` instead of `python3`.
Add --sensor-leds to a tcs3448 or dual build to also light the sensor boards' white LEDs,
and --sensor-led-ma N (4-258, default 20) to set their brightness.

Builds run in Docker (espressif/idf), so no ESP-IDF install is needed. Flashing
and the monitor run on this computer with esptool, because Docker Desktop on
macOS and Windows can't reach USB. esptool is installed into .venv/ on first use.
"""
import argparse
import os
import shlex
import shutil
import subprocess
import sys
from pathlib import Path

IMAGE = "espressif/idf:v5.5.5"  # same as CI
REPO = Path(__file__).resolve().parent.parent
FIRMWARE = REPO / "firmware"
VENV = REPO / ".venv"

# Build name -> sdkconfig files layered on top of sdkconfig.defaults.
BUILDS = {
    "mock": [],
    "dual": ["sdkconfig.dual"],
    "dual-ble": ["sdkconfig.dual", "sdkconfig.ble"],
    "tcs3448": ["sdkconfig.tcs3448"],
    "ble": ["sdkconfig.ble"],
    "tcs3448-ble": ["sdkconfig.tcs3448", "sdkconfig.ble"],
}


def run(cmd, cwd=None, echo=True):
    if echo:
        print("+", shlex.join(str(c) for c in cmd), flush=True)
    try:
        code = subprocess.run(cmd, cwd=cwd).returncode
    except FileNotFoundError:
        sys.exit(f"error: {cmd[0]} not found. Is it installed and on your PATH? See docs/setup.md")
    except KeyboardInterrupt:
        sys.exit(130)
    if code != 0:
        sys.exit(code)


def docker(mount, workdir, args, tty=False):
    if shutil.which("docker") is None:
        sys.exit("error: docker not found. Install Docker (docs/setup.md, step 1) and make sure it's running.")
    cmd = ["docker", "run", "--rm"]
    if tty:
        cmd.append("-it")
    if hasattr(os, "getuid"):  # Linux/macOS: keep build files owned by you, not root
        cmd += ["-u", f"{os.getuid()}:{os.getgid()}", "-e", "HOME=/tmp"]
    cmd += ["-v", f"{mount}:{workdir}", "-w", workdir, IMAGE, *args]
    run(cmd)


def idf_args(build, extra=()):
    # Own build dir and sdkconfig per build (and per flag set), so switching never needs a clean.
    name = build_name(build, extra)
    layers = [*BUILDS[build], *extra]
    if "sdkconfig.sensor_leds" in extra:
        layers.append(f"build-{name}/sdkconfig.led_ma")  # written by set_led_ma()
    args = ["idf.py", "-B", f"build-{name}", "-D", f"SDKCONFIG=build-{name}/sdkconfig"]
    if layers:
        args += ["-D", "SDKCONFIG_DEFAULTS=" + ";".join(["sdkconfig.defaults", *layers])]
    return args


def build_name(build, extra=()):
    return build + ("-leds" if "sdkconfig.sensor_leds" in extra else "")


def venv_python():
    """Python inside .venv/ with esptool installed, creating it on first use."""
    py = VENV / ("Scripts/python.exe" if os.name == "nt" else "bin/python")
    if not py.exists():
        print("Setting up .venv/ with esptool (first run only)...")
        run([sys.executable, "-m", "venv", str(VENV)])
        run([str(py), "-m", "pip", "install", "--quiet", "esptool"])
    return str(py)


def set_led_ma(name, ma):
    """Apply --sensor-led-ma, or the default when it's left off.

    A build's saved sdkconfig wins over the defaults files, so drop the saved
    line every time; the value then comes from this file, or the Kconfig default.
    """
    build_dir = FIRMWARE / f"build-{name}"
    build_dir.mkdir(exist_ok=True)
    saved = build_dir / "sdkconfig"
    if saved.exists():
        lines = saved.read_text().splitlines(keepends=True)
        saved.write_text("".join(l for l in lines if not l.startswith("CONFIG_DC_SENSOR_LED_MA=")))
    (build_dir / "sdkconfig.led_ma").write_text(f"CONFIG_DC_SENSOR_LED_MA={ma}\n" if ma else "")


def flash(build, port):
    run([venv_python(), "-m", "esptool", "--chip", "esp32", "-p", port, "-b", "460800",
         "write-flash", "@flash_args"], cwd=FIRMWARE / f"build-{build}")


def monitor(port):
    print("Serial monitor: press l or c to measure, EN/RST on the board to reboot. Quit with Ctrl+].")
    run([venv_python(), "-m", "serial.tools.miniterm", port, "115200"])


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("target", choices=[*BUILDS, "monitor", "ports", "qemu", "test"])
    p.add_argument("port", nargs="?", help="serial port, e.g. /dev/ttyUSB0, /dev/cu.usbserial-0001, COM3")
    p.add_argument("--sensor-leds", action="store_true",
                   help="also light the sensor boards' onboard white LEDs (tcs3448 and dual builds)")
    p.add_argument("--sensor-led-ma", type=int, choices=range(4, 259), metavar="N",
                   help="white LED current in mA at 100%% intensity, 4-258 (default 20); lower is dimmer")
    a = p.parse_args()
    extra = []
    if a.sensor_leds:
        if "sdkconfig.tcs3448" not in BUILDS.get(a.target, []) and "sdkconfig.dual" not in BUILDS.get(a.target, []):
            p.error("--sensor-leds needs a real-sensor build (tcs3448, tcs3448-ble, dual, dual-ble)")
        extra.append("sdkconfig.sensor_leds")
        set_led_ma(build_name(a.target, extra), a.sensor_led_ma)
    elif a.sensor_led_ma is not None:
        p.error("--sensor-led-ma needs --sensor-leds")

    if a.target == "ports":
        # Only USB ports; Linux also lists dozens of legacy /dev/ttyS* with no hardware.
        run([venv_python(), "-c", "from serial.tools import list_ports as l; "
             "[print(p.device, '-', p.description) for p in l.comports() if p.hwid != 'n/a'] "
             "or print('No USB serial ports found. Check the cable and driver (docs/setup.md).')"],
            echo=False)
    elif a.target == "monitor":
        if not a.port:
            p.error("monitor needs a PORT (find it with: tools/run.py ports)")
        monitor(a.port)
    elif a.target == "qemu":
        docker(FIRMWARE, "/project", [*idf_args("mock"), "build", "qemu", "monitor"], tty=True)
    elif a.target == "test":
        docker(REPO, "/repo", ["bash", "-c", "bash tools/check_layers.sh"
                      " && cmake -S firmware/test/host -B build/host-docker"
                      " && cmake --build build/host-docker"
                      " && ctest --test-dir build/host-docker --output-on-failure"])
    else:
        name = build_name(a.target, extra)
        docker(FIRMWARE, "/project", [*idf_args(a.target, extra), "build"])
        if a.port:
            flash(name, a.port)
            monitor(a.port)
        else:
            print(f"Built firmware/build-{name}. Add a PORT to flash it.")


if __name__ == "__main__":
    main()
