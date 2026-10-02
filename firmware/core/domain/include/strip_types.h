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

typedef struct {
    strip_mode_t mode;
    strip_verdict_t verdict;
    float score;          // placeholder metric, see strip_analysis.c. Lateral flow: T line signal
    float control_score;  // lateral flow: C line signal. 0 otherwise
    size_t sample_count;
} strip_result_t;

typedef struct {
    strip_mode_t mode;
    // Colorimetric (placeholder analysis).
    float positive_threshold;  // TODO(team): placeholder
    float min_signal;          // below this, result is STRIP_INVALID
    // Lateral flow. A line counts as present when its signal is >= its threshold.
    float t_threshold;         // TODO(team): placeholder
    float c_threshold;         // TODO(team): placeholder. No C line = STRIP_INVALID
    lfa_format_t lfa_format;
} strip_config_t;
