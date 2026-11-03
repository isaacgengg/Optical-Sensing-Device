// Domain types for test strip readings and results.
// Pure data, no dependencies. Everything else in the project builds on these.
#pragma once
#include <stddef.h>

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
    strip_verdict_t verdict;
    float reflectance[DC_MAX_CHANNELS];
} color_ref_t;

typedef struct {
    strip_mode_t mode;
    strip_verdict_t verdict;
    // Colorimetric: RMS reflectance distance to the closest reference (lower =
    // closer). Lateral flow: T line signal (placeholder metric).
    float score;
    float control_score;  // lateral flow: C line signal. 0 otherwise
    size_t sample_count;
    const char *match;              // colorimetric: closest reference's name, NULL if none
    channel_reading_t raw;          // averaged raw channels (lateral flow: T line)
    channel_reading_t reflectance;  // colorimetric: raw / white. count 0 otherwise
} strip_result_t;

typedef struct {
    strip_mode_t mode;
    // Colorimetric: reflectance = reading / white, then nearest reference.
    const channel_reading_t *white;  // white card, read at the same settings
    const color_ref_t *refs;
    size_t ref_count;
    float max_distance;  // RMS distance; farther than this from every ref = STRIP_INVALID
    // Lateral flow. A line counts as present when its signal is >= its threshold.
    float t_threshold;         // TODO(team): placeholder
    float c_threshold;         // TODO(team): placeholder. No C line = STRIP_INVALID
    lfa_format_t lfa_format;
} strip_config_t;
