// LFA zone calibration. NEVER copy the colorimetric paper-color references.
// Measure dark/blank in the final two-zone rig at the settings below. Choose
// visible channel bits (0..10), then fit thresholds using labeled strips.
// Zero-initialized references deliberately make hardware LFA INVALID while
// still logging both raw spectra. See docs/dual-sensor-lfa.md.
#pragma once
#include "strip_types.h"
#define LFA_TEST_INTEGRATION_MS 100
#define LFA_CONTROL_INTEGRATION_MS 100
#define LFA_TEST_GAIN 256
#define LFA_CONTROL_GAIN 256
#if CONFIG_DC_SENSOR_MOCK
// Synthetic demo only; not a hardware calibration.
static const lfa_zone_ref_t LFA_TEST_REF = {
    .dark = {.count = 14}, .blank = {.values = {[4] = 2000}, .count = 14},
    .channel_mask = 1u << 4, .threshold = 0.3f,
};
static const lfa_zone_ref_t LFA_CONTROL_REF = {
    .dark = {.count = 14}, .blank = {.values = {[4] = 1000}, .count = 14},
    .channel_mask = 1u << 4, .threshold = 0.3f,
};
#else
static const lfa_zone_ref_t LFA_TEST_REF = {0};
static const lfa_zone_ref_t LFA_CONTROL_REF = {0};
#endif
