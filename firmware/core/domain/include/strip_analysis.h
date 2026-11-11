// Pure strip-analysis rules: numbers in, results out. No I/O, no timing.
#pragma once
#include <stddef.h>
#include "strip_types.h"

// Per-channel mean of `n` samples. Returns a reading with count = 0 if n == 0.
channel_reading_t strip_average(const channel_reading_t *samples, size_t n);

// Colorimetric: averages the samples, divides by cfg->white to get reflectance,
// and returns the verdict of the closest cfg->refs entry. STRIP_INVALID if the
// config is incomplete, channel counts differ, or no ref is within max_distance.
strip_result_t strip_analyze(const channel_reading_t *samples, size_t n,
                             const strip_config_t *cfg);

// Lateral flow: separately calibrated contrast from `n` samples per zone.
// Missing/invalid calibration or absent C line => INVALID. Otherwise the T
// line decides per cfg->lfa_format. Both raw spectra are retained.
strip_result_t strip_analyze_lateral_flow(const channel_reading_t *t_samples,
                                          const channel_reading_t *c_samples,
                                          size_t n, const strip_config_t *cfg);
