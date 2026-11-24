// LFA references for the 2026-11-23 red-marker paper demonstration.
// Dark: batch10; blank: batch12; thresholds: batches11-15; stationary check:
// batch16. Placement repeatability and real assays are not validated.
// These references are independent of color_refs.h. See docs/dual-sensor-lfa.md.
#pragma once
#include "strip_types.h"
#define LFA_TEST_INTEGRATION_MS 100
#define LFA_CONTROL_INTEGRATION_MS 100
// Measured settings after saturation at 256x with the 50 mA rig.
#define LFA_TEST_GAIN 128
#define LFA_CONTROL_GAIN 64
#if CONFIG_DC_SENSOR_MOCK
// Synthetic demo only; not a hardware calibration.
#define LFA_PROFILE_ID "synthetic-mock"
static const lfa_zone_ref_t LFA_TEST_REF = {
    .dark = {.count = 14}, .blank = {.values = {[4] = 2000}, .count = 14},
    .channel_mask = 1u << 4, .threshold = 0.3f,
};
static const lfa_zone_ref_t LFA_CONTROL_REF = {
    .dark = {.count = 14}, .blank = {.values = {[4] = 1000}, .count = 14},
    .channel_mask = 1u << 4, .threshold = 0.3f,
};
#elif CONFIG_DC_SENSOR_TCS3448 && CONFIG_DC_LFA_AS7343 && \
      CONFIG_DC_SENSOR_LEDS && CONFIG_DC_SENSOR_LED_MA == 50 && \
      LFA_TEST_GAIN == 128 && LFA_CONTROL_GAIN == 64 && \
      LFA_TEST_INTEGRATION_MS == 100 && LFA_CONTROL_INTEGRATION_MS == 100
#define LFA_PROFILE_ID "paper-lfa-20261123-v1"
// F1 F2 FZ F3 F4 F5 FY FXL F6 F7 F8 NIR VIS FD. Only selected visible
// channels enter the contrast score; NIR, VIS and FD remain excluded.
static const lfa_zone_ref_t LFA_TEST_REF = {
    .dark = {.values = {[13] = 3.0f}, .count = 14},
    .blank = {.values = {
        29.0f, 93.666667f, 539.0f, 578.0f, 554.5f, 199.5f, 692.5f,
        616.5f, 532.833333f, 352.833333f, 51.0f, 28.0f, 844.666667f,
        18002.666667f,
    }, .count = 14},
    .channel_mask = (1u << 1) | (1u << 2) | (1u << 4), // F2, FZ, F4
    .threshold = 0.228570715f,
};
static const lfa_zone_ref_t LFA_CONTROL_REF = {
    .dark = {.values = {[13] = 0.9f}, .count = 14},
    .blank = {.values = {
        19.0f, 42.0f, 178.166667f, 413.833333f, 256.666667f, 132.0f,
        342.666667f, 348.833333f, 385.0f, 224.5f, 28.0f, 19.666667f,
        680.166667f, 24047.0f,
    }, .count = 14},
    .channel_mask = (1u << 3) | (1u << 5), // F3, F5
    .threshold = 0.018411562f,
};
#else
// Other lighting/acquisition configurations have no measured calibration.
#define LFA_PROFILE_ID "uncalibrated"
static const lfa_zone_ref_t LFA_TEST_REF = {0};
static const lfa_zone_ref_t LFA_CONTROL_REF = {0};
#endif
