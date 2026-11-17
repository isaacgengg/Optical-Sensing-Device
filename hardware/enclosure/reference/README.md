# Unchanged mechanical references

`ESP32.step`, `LCD.step`, and `Header.step` were extracted from
`Electronics Assembly.step` at commit
`c057e6e703f2676648b67805145a647a3632447e` (PR #22). These small neutral
references let the cross-platform generator rebuild without reading an older
full assembly or requiring Inventor.

ESP32 and LCD retain their assembly coordinates and their original assumed
dimensions. The header was transformed back to the Adafruit breakout's local
frame (VIN at X 6.35, header Y 2.54). The original vendor breakout STEP is read
directly from `Purchased Parts/Adafruit/`.

They are mechanical context, not newly measured dimensions or electrical
models. The new component-side photos do not verify the solder-side routes,
LCD mounting, connector clearance, or spacer heights.
