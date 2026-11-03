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

// TODO(team): MADE UP. Shaped like a white LED off a white card (blue LED peak
// at FZ, broad yellow hump, little NIR). Replace with a measured white card.
static const channel_reading_t COLOR_WHITE = {
    .values = {179, 422, 1649, 1103, 763, 1046, 1380, 1416, 977, 517, 211, 90, 3113, 140},
    .count = 14,
};

// TODO(team): MADE UP. Replace with measured pad colors (and real names) from
// DetectaChem's color chart or known samples.
static const color_ref_t COLOR_REFS[] = {
    {   // unreacted pad: near-white, flat
        .name = "clear", .verdict = STRIP_NEGATIVE,
        .reflectance = {0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85},
    },
    {   // purple: absorbs green, reflects blue and red
        .name = "purple", .verdict = STRIP_POSITIVE,
        .reflectance = {0.55, 0.50, 0.45, 0.35, 0.20, 0.15, 0.18, 0.35, 0.60, 0.75, 0.80, 0.85, 0.40, 0.40},
    },
    {   // yellow-orange: absorbs blue, reflects green through red
        .name = "orange", .verdict = STRIP_POSITIVE,
        .reflectance = {0.10, 0.10, 0.12, 0.20, 0.45, 0.65, 0.65, 0.80, 0.85, 0.85, 0.85, 0.85, 0.60, 0.60},
    },
};

// TODO(team): placeholder. RMS reflectance difference allowed before the pad
// counts as "no known color" (INVALID). 0.15 = off by ~15% per channel.
#define COLOR_MAX_DISTANCE 0.15f
