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

// Measured 2026-11-05, cardboard-box rig, 100 ms @ 256x:
// mean of 2 white-card reads (a third, 22% dimmer, was dropped as misplaced).
static const channel_reading_t COLOR_WHITE = {
    .values = {14, 84.5, 394.5, 289, 455.5, 173.5, 543, 385.5, 242.5, 123, 17, 18, 556, 12031.5},
    .count = 14,
};

// Measured 2026-11-05, same rig: mean reflectance of 4 red and 3 green test
// strips. Low-count channels (F1, F8, NIR) are noisy until there's more light.
// TODO(team): placeholder mapping (red = fentanyl, green = cocaine) for
// test-paper colors, not real reagent reactions.
static const color_ref_t COLOR_REFS[] = {
    {   // TODO(team): white card, all 1.0 by definition. Replace with the
        // refl= line of a few real unused pads; they're rarely pure white.
        .name = "control", .color = "white", .verdict = STRIP_NEGATIVE,
        .reflectance = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    },
    {
        .name = "fentanyl", .color = "red", .verdict = STRIP_POSITIVE,
        .reflectance = {0.089, 0.050, 0.032, 0.046, 0.037, 0.035, 0.067, 0.140, 0.387, 0.457, 0.471, 0.389, 0.127, 0.160},
    },
    {
        .name = "cocaine", .color = "green", .verdict = STRIP_POSITIVE,
        .reflectance = {0.310, 0.312, 0.349, 0.373, 0.528, 0.402, 0.366, 0.237, 0.139, 0.374, 0.843, 0.352, 0.344, 0.332},
    },
};

// RMS reflectance difference allowed before the pad counts as "no known
// color" (INVALID). Measured on the box rig: same color 0.01-0.09 apart,
// red vs green >= 0.27, white vs either >= 0.42. TODO(team): re-check on the printed rig.
#define COLOR_MAX_DISTANCE 0.15f
