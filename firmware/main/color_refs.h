// Five-color bench calibration, 2026-11-23. TCS3448 test/color sensor.
// Measured with both onboard LEDs at 50 mA, 100 ms integration and 256x gain.
// Channel order: F1 F2 FZ F3 F4 F5 FY FXL F6 F7 F8 NIR VIS FD.
// The existing 14-channel, equal-weight shape calculation is retained.
//
// Source: 28 user-labeled averaged serial readings; recorded raw values were
// rounded to integers. The raw captures and provenance are in
// test/host/data/five_color_2026_11_23.h and its README. Each reference is its
// mean raw vector divided by the mean of the five white captures below.
// Tape-covered green and the earlier yellow positioning batch are excluded.
// Recalibrate all references together if illumination or geometry changes.
#pragma once
#include "strip_types.h"

#define COLOR_PROFILE_ID "bench5-20261123-map1"
// Provisional retest limits. Largest distance to its own fitted reference was
// 0.005; closest reference pair is turquoise/deep_purple at 0.036. The lead
// requirement rejects close calls even inside the absolute distance cutoff.
// These captures fit the references; fresh repositioned samples must validate them.
#define COLOR_MAX_DISTANCE 0.025f
#define COLOR_MIN_MARGIN 0.010f

static const channel_reading_t COLOR_WHITE = {
    .values = {53.00000000f, 168.40000000f, 953.80000000f, 1033.40000000f, 941.40000000f, 335.00000000f, 1199.20000000f, 1050.40000000f, 930.60000000f, 625.60000000f, 93.40000000f, 48.00000000f, 1508.00000000f, 31123.80000000f},
    .count = 14,
};

// User-selected display mappings, 2026-11-23. Color references and thresholds
// are unchanged; these labels do not establish chemical identity.
// Retain the existing bench verdict convention: white = NEGATIVE control,
// a recognized colored reference = POSITIVE, rejected/ambiguous = INVALID.
// These verdicts are not validated assay interpretations.
static const color_ref_t COLOR_REFS[] = {
    {
        .name = "control", .color = "white", .verdict = STRIP_NEGATIVE,
        .max_distance = COLOR_MAX_DISTANCE,
        .reflectance = {1.00000000f, 1.00000000f, 1.00000000f, 1.00000000f, 1.00000000f, 1.00000000f, 1.00000000f, 1.00000000f, 1.00000000f, 1.00000000f, 1.00000000f, 1.00000000f, 1.00000000f, 1.00000000f},
    },
    {
        .name = "psychedelics", .color = "yellow", .verdict = STRIP_POSITIVE,
        .max_distance = COLOR_MAX_DISTANCE,
        .reflectance = {0.94339623f, 0.71852732f, 0.66156427f, 0.78317528f, 0.85015226f, 0.88358209f, 0.91060707f, 0.94027672f, 0.99362419f, 0.99744246f, 1.00642398f, 0.97916667f, 0.88373121f, 0.95230445f},
    },
    {
        .name = "fentanyl", .color = "turquoise", .verdict = STRIP_POSITIVE,
        .max_distance = COLOR_MAX_DISTANCE,
        .reflectance = {0.92452830f, 0.77197150f, 0.75400154f, 0.84768725f, 0.82607464f, 0.79253731f, 0.82888592f, 0.79271389f, 0.80145426f, 0.83280051f, 0.89935760f, 0.87500000f, 0.80713970f, 0.88821095f},
    },
    {
        .name = "cocaine", .color = "orange", .verdict = STRIP_POSITIVE,
        .max_distance = COLOR_MAX_DISTANCE,
        .reflectance = {0.92452830f, 0.70665083f, 0.64706088f, 0.76914393f, 0.74746831f, 0.77412935f, 0.83180454f, 0.87093806f, 0.97643098f, 0.98811807f, 0.99571734f, 0.95833333f, 0.82979664f, 0.91455199f},
    },
    {
        .name = "explosives", .color = "deep_purple", .verdict = STRIP_POSITIVE,
        .max_distance = COLOR_MAX_DISTANCE,
        .reflectance = {0.91037736f, 0.72669240f, 0.67938771f, 0.78829592f, 0.72843637f, 0.75783582f, 0.79761508f, 0.78589109f, 0.79934988f, 0.82960358f, 0.88998929f, 0.86458333f, 0.76931366f, 0.87233484f},
    },
};
