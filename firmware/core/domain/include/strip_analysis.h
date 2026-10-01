// Pure strip-analysis rules: numbers in, results out. No I/O, no timing.
#pragma once
#include <stddef.h>
#include "strip_types.h"

// Per-channel mean of `n` samples. Returns a reading with count = 0 if n == 0.
channel_reading_t strip_average(const channel_reading_t *samples, size_t n);

// Turns a set of samples into a verdict. See strip_analysis.c (placeholder).
strip_result_t strip_analyze(const channel_reading_t *samples, size_t n,
                             const strip_config_t *cfg);
