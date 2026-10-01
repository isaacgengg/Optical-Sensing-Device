// Domain types for test strip readings and results.
// Pure data, no dependencies. Everything else in the project builds on these.
#pragma once
#include <stddef.h>

#define DC_MAX_CHANNELS 16

typedef struct {
    float values[DC_MAX_CHANNELS];
    size_t count;
} channel_reading_t;

typedef enum {
    STRIP_NEGATIVE = 0,
    STRIP_POSITIVE,
    STRIP_INVALID,
} strip_verdict_t;

typedef struct {
    strip_verdict_t verdict;
    float score;          // placeholder metric, see strip_analysis.c
    size_t sample_count;
} strip_result_t;

typedef struct {
    float positive_threshold;  // TODO(team): placeholder
    float min_signal;          // below this, result is STRIP_INVALID
} strip_config_t;
