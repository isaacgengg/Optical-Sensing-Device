// Domain types for test strip readings and results.
// Pure data, no dependencies. Everything else in the project builds on these.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define DC_MAX_CHANNELS 16

typedef struct {
    float values[DC_MAX_CHANNELS];
    size_t count;
} channel_reading_t;

// Which kind of test is being run. Chosen by the user before each measurement.
typedef enum {
    STRIP_MODE_LATERAL_FLOW = 0,
    STRIP_MODE_COLORIMETRIC,
} strip_mode_t;

// Lateral flow strip format. Standard (sandwich): a visible T line means the
// target is present. Competitive (common for drugs): the target blocks the T
// line, so a visible T line means it is absent.
typedef enum {
    LFA_STANDARD = 0,
    LFA_COMPETITIVE,
} lfa_format_t;

typedef enum {
    STRIP_NEGATIVE = 0,
    STRIP_POSITIVE,
    STRIP_INVALID,
} strip_verdict_t;

// One known pad color: its reflectance per channel (0 = absorbs everything,
// 1 = reflects like the white reference) and the verdict it stands for.
typedef struct {
    const char *name;
    const char *color;  // what it looks like, e.g. "red". For the log
    strip_verdict_t verdict;
    float reflectance[DC_MAX_CHANNELS];
    // RMS shape cutoff for this reference; 0 inherits strip_config_t.max_distance.
    float max_distance;
} color_ref_t;

typedef enum {
    COLOR_MATCH_NOT_EVALUATED = 0,
    COLOR_MATCH_ACCEPTED,
    COLOR_MATCH_TOO_FAR,
    COLOR_MATCH_AMBIGUOUS,
} color_match_status_t;

// Diagnostics retain the closest candidates even when color/match are rejected.
typedef struct {
    color_match_status_t status;
    const char *nearest;
    const char *runner_up;
    float runner_up_score;
    float margin;
    float max_distance;
    float min_margin;
} color_match_info_t;

// Independently measured at each zone's gain, exposure, illumination and geometry.
// Bit i selects channel i; only visible channels 0..10 may be selected.
typedef struct {
    channel_reading_t dark;
    channel_reading_t blank;
    uint16_t channel_mask;
    float threshold;  // 0..1 contrast; larger means a darker line
} lfa_zone_ref_t;

// A lights-off calibration capture. Kept separate from strip_result_t so it
// cannot be classified or sent as an assay result through a result sink.
typedef struct {
    channel_reading_t test_raw;
    channel_reading_t control_raw;
    size_t sample_count;
    uint16_t test_gain;
    uint16_t control_gain;
    uint16_t test_integration_ms;
    uint16_t control_integration_ms;
} lfa_dark_capture_t;

typedef struct {
    strip_mode_t mode;
    strip_verdict_t verdict;
    // Colorimetric: RMS distance between mean-normalized reflectance shapes of
    // the reading and the closest reference (lower =
    // closer). Lateral flow: calibrated T contrast (0 if unavailable; verdict INVALID).
    float score;
    float control_score;  // lateral flow: C line signal. 0 otherwise
    size_t sample_count;
    const char *match;              // colorimetric: closest reference's name, NULL if none
    const char *color;              // colorimetric: closest reference's color, NULL if none
    channel_reading_t raw;          // averaged raw channels (lateral flow: T line)
    channel_reading_t control_raw;  // LFA second/control zone
    bool lfa_calibration_valid;
    channel_reading_t reflectance;  // colorimetric: raw / white. count 0 otherwise
    color_match_info_t color_match;
} strip_result_t;

typedef struct {
    strip_mode_t mode;
    // Colorimetric: reflectance = reading / white, then nearest reference by
    // spectral shape (each divided by its own channel mean).
    const channel_reading_t *white;  // white card, read at the same settings
    const color_ref_t *refs;
    size_t ref_count;
    float max_distance;  // Default RMS shape cutoff when the nearest ref has no override.
    float min_color_margin;  // Required runner-up distance minus best distance; 0 disables.
    // LFA requires BOTH independent zone references; NULL means uncalibrated.
    const lfa_zone_ref_t *test_ref;
    const lfa_zone_ref_t *control_ref;
    lfa_format_t lfa_format;
} strip_config_t;
