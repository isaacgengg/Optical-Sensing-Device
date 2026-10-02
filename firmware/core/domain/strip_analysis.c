// TODO(team): PLACEHOLDER ALGORITHM.
// The real analysis depends on the optical design (e.g. whether test and
// control lines are measured separately) and on calibration data.
// Replace this once the sensing approach is decided. Keep it pure.

#include "strip_analysis.h"

channel_reading_t strip_average(const channel_reading_t *samples, size_t n)
{
    channel_reading_t avg = {0};
    if (samples == NULL || n == 0) {
        return avg;
    }

    avg.count = samples[0].count > DC_MAX_CHANNELS ? DC_MAX_CHANNELS : samples[0].count;
    for (size_t i = 0; i < n; i++) {
        for (size_t c = 0; c < avg.count; c++) {
            avg.values[c] += samples[i].values[c];
        }
    }
    for (size_t c = 0; c < avg.count; c++) {
        avg.values[c] /= (float)n;
    }
    return avg;
}

strip_result_t strip_analyze(const channel_reading_t *samples, size_t n,
                             const strip_config_t *cfg)
{
    strip_result_t result = {.verdict = STRIP_INVALID, .score = 0.0f, .sample_count = n};
    if (samples == NULL || cfg == NULL || n == 0) {
        return result;
    }
    result.mode = cfg->mode;
    // ponytail: both modes share the placeholder below. Branch on cfg->mode
    // once the lateral flow (T/C) and colorimetric algorithms exist.

    channel_reading_t avg = strip_average(samples, n);
    if (avg.count == 0) {
        return result;
    }

    float sum = 0.0f;
    for (size_t c = 0; c < avg.count; c++) {
        sum += avg.values[c];
    }
    result.score = sum / (float)avg.count;

    if (result.score < cfg->min_signal) {
        result.verdict = STRIP_INVALID;
    } else if (result.score >= cfg->positive_threshold) {
        result.verdict = STRIP_POSITIVE;
    } else {
        result.verdict = STRIP_NEGATIVE;
    }
    return result;
}
