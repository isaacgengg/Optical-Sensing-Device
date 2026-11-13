# Hardware

This folder holds the PCB design files (KiCad), the enclosure CAD
(Inventor, in `enclosure/`), the BOM, fab outputs
(Gerbers, drill files, pick-and-place) for each board revision, and the
datasheets for the parts we use.

`firmware/main/board.h` must match the current schematic. When a pin, bus, or
I2C address changes here, update `board.h` in the same commit.

**[WIRING.md](WIRING.md)** is the build guide for the perf board: where the
ESP32, TCS3448, AS7343 and LCD go, pin maps, a cut list with routes, build
order and checks.

## Board revisions

| Rev | Date | Fab submitted | Notes |
|---|---|---|---|
| | | | |

Current photo-layout enclosure STEP/STL files and setup are in
[enclosure/README.md](enclosure/README.md). Native Inventor files still need
the documented Windows rebuild; use the new neutral exports for this prototype.
