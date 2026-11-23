// Colorimetric reference data: the white calibration and the known pad colors.
// Included only by main.c.
//
// Channel order matches the TCS3448 driver (adapters/tcs3448/include/tcs3448.h):
//   F1   F2   FZ   F3   F4   F5   FY   FXL  F6   F7   F8   NIR  VIS  FD
//   405  425  450  475  515  550  555  600  640  690  745  855  (broad) (flicker)  nm
//
// To fill these in with real numbers, in the jig, with the colorimetric
// measurement settings in main.c:
//   1. Put a white card where the pad goes and press 'c'. Copy the "raw="
//      log line into COLOR_WHITE.
//   2. Reflash, then put each known pad color in and press 'c'. Copy its
//      "refl=" line into a COLOR_REFS entry.
// Re-do step 1 whenever the LED, gain, integration time or geometry changes.

#pragma once
#include "strip_types.h"

// Measured 2026-11-23, 100 ms @ 256x: mean of 4 white-card reads.
static const channel_reading_t COLOR_WHITE = {
    .values = {51, 130.25, 680.25, 879.75, 716.5, 300.25, 1046.75, 935.75, 876.25, 567.75, 82.25, 50.5, 1280, 35193},
    .count = 14,
};

// Measured 2026-11-23, same settings: mean of 5 reads per color, divided by
// COLOR_WHITE above (recomputed from raw=, since the logged refl= was against
// the old white). Read-to-read spread <= 0.008 RMS.
// TODO(team): placeholder mapping (red = fentanyl, green = cocaine, yellow =
// psychedelics, turquoise = general explosives) for test-paper colors, not
// real reagent reactions.
static const color_ref_t COLOR_REFS[] = {
    {   // TODO(team): white card, all 1.0 by definition. Replace with the
        // refl= line of a few real unused pads; they're rarely pure white.
        .name = "control", .color = "white", .verdict = STRIP_NEGATIVE,
        .reflectance = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    },
    {
        .name = "fentanyl", .color = "red", .verdict = STRIP_POSITIVE,
        .reflectance = {0.906, 0.662, 0.577, 0.754, 0.687, 0.671, 0.706, 0.828, 0.978, 1.011, 0.987, 0.891, 0.862, 0.749},
    },
    {
        .name = "cocaine", .color = "green", .verdict = STRIP_POSITIVE,
        .reflectance = {0.843, 0.590, 0.514, 0.655, 0.754, 0.625, 0.638, 0.626, 0.604, 0.666, 0.749, 0.610, 0.640, 0.583},
    },
    {
        .name = "psychedelics", .color = "yellow", .verdict = STRIP_POSITIVE,
        .reflectance = {0.792, 0.534, 0.413, 0.533, 0.756, 0.682, 0.740, 0.808, 0.808, 0.828, 0.795, 0.832, 0.738, 0.632},
    },
    {
        .name = "explosives", .color = "turquoise", .verdict = STRIP_POSITIVE,
        .reflectance = {0.843, 0.691, 0.669, 0.727, 0.756, 0.603, 0.626, 0.601, 0.587, 0.658, 0.729, 0.598, 0.661, 0.586},
    },
};

// RMS shape distance (each spectrum divided by its own mean, see
// strip_analysis.c) allowed before the pad counts as "no known color"
// (INVALID). From the 2026-11-23 refs: closest pair green vs turquoise 0.080,
// everything else >= 0.11; a green re-read 25% dimmer landed 0.028 from green.
// Kept at half the closest gap. Shape matching can't tell white from an even
// grey. TODO(team): re-tune once the rig is stable and real pads are read.
#define COLOR_MAX_DISTANCE 0.04f
