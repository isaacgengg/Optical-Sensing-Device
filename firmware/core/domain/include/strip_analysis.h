// Pure strip-analysis rules: numbers in, results out. No I/O, no timing.
#pragma once
#include <stddef.h>
#include "strip_types.h"

// Per-channel mean of `n` samples. Returns a reading with count = 0 if n == 0.
channel_reading_t strip_average(const channel_reading_t *samples, size_t n);

// Turns a set of samples into a verdict. Used for colorimetric strips until
// the real color analysis exists. See strip_analysis.c (placeholder).
strip_result_t strip_analyze(const channel_reading_t *samples, size_t n,
                             const strip_config_t *cfg);

// Lateral flow: `n` samples of the T line and `n` of the C line. No C line
// means STRIP_INVALID; otherwise the T line decides, per cfg->lfa_format.
strip_result_t strip_analyze_lateral_flow(const channel_reading_t *t_samples,
                                          const channel_reading_t *c_samples,
                                          size_t n, const strip_config_t *cfg);
