#!/usr/bin/env python3
"""Build, flash and monitor the firmware with one command, on Linux, macOS or Windows.

    python3 tools/run.py tcs3448 PORT   build, flash, then open the serial monitor
    python3 tools/run.py tcs3448        build only
    python3 tools/run.py monitor PORT   reopen the serial monitor (no build, no flash)
    python3 tools/run.py ports          list serial ports, to find PORT
    python3 tools/run.py qemu           run the mock build in the QEMU emulator (no board)
    python3 tools/run.py test           host tests and layer check

Builds: mock, tcs3448, ble, tcs3448-ble. On Windows use `py` instead of `python3`.

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


def idf_args(build):
    # Own build dir and sdkconfig per build, so switching never needs a clean.
    args = ["idf.py", "-B", f"build-{build}", "-D", f"SDKCONFIG=build-{build}/sdkconfig"]
    if BUILDS[build]:
        args += ["-D", "SDKCONFIG_DEFAULTS=" + ";".join(["sdkconfig.defaults", *BUILDS[build]])]
    return args


def venv_python():
    """Python inside .venv/ with esptool installed, creating it on first use."""
    py = VENV / ("Scripts/python.exe" if os.name == "nt" else "bin/python")
    if not py.exists():
        print("Setting up .venv/ with esptool (first run only)...")
        run([sys.executable, "-m", "venv", str(VENV)])
        run([str(py), "-m", "pip", "install", "--quiet", "esptool"])
    return str(py)


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
    a = p.parse_args()

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
        docker(FIRMWARE, "/project", [*idf_args(a.target), "build"])
        if a.port:
            flash(a.target, a.port)
            monitor(a.port)
        else:
            print(f"Built firmware/build-{a.target}. Add a PORT to flash it.")


if __name__ == "__main__":
    main()
