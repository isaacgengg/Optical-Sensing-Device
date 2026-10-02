// TODO(team): PLACEHOLDER SIGNAL METRIC.
// A line's "signal" is the mean across channels (mean_signal below). A real
// line darkens the strip, so the real metric is likely absorbance against
// blank membrane in the bands the line absorbs (gold: ~520-540 nm), plus
// dark/white calibration. The lateral flow C-then-T rules are real; the
// colorimetric path (strip_analyze) is a placeholder until DetectaChem's
// color chart exists. Keep it pure.

#include <stdbool.h>
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

// Mean across channels of the averaged samples. False if there are no channels.
static bool mean_signal(const channel_reading_t *samples, size_t n, float *out)
{
    channel_reading_t avg = strip_average(samples, n);
    if (avg.count == 0) {
        return false;
    }
    float sum = 0.0f;
    for (size_t c = 0; c < avg.count; c++) {
        sum += avg.values[c];
    }
    *out = sum / (float)avg.count;
    return true;
}

strip_result_t strip_analyze(const channel_reading_t *samples, size_t n,
                             const strip_config_t *cfg)
{
    strip_result_t result = {.verdict = STRIP_INVALID, .score = 0.0f, .sample_count = n};
    if (samples == NULL || cfg == NULL || n == 0) {
        return result;
    }
    result.mode = cfg->mode;

    if (!mean_signal(samples, n, &result.score)) {
        return result;
    }

    if (result.score < cfg->min_signal) {
        result.verdict = STRIP_INVALID;
    } else if (result.score >= cfg->positive_threshold) {
        result.verdict = STRIP_POSITIVE;
    } else {
        result.verdict = STRIP_NEGATIVE;
    }
    return result;
}

strip_result_t strip_analyze_lateral_flow(const channel_reading_t *t_samples,
                                          const channel_reading_t *c_samples,
                                          size_t n, const strip_config_t *cfg)
{
    strip_result_t result = {
        .mode = STRIP_MODE_LATERAL_FLOW, .verdict = STRIP_INVALID, .sample_count = n};
    if (t_samples == NULL || c_samples == NULL || cfg == NULL || n == 0) {
        return result;
    }
    if (!mean_signal(t_samples, n, &result.score) ||
        !mean_signal(c_samples, n, &result.control_score)) {
        return result;
    }

    // No C line: the sample didn't flow, so the T line means nothing.
    if (result.control_score < cfg->c_threshold) {
        return result;
    }

    bool t_line = result.score >= cfg->t_threshold;
    bool positive = cfg->lfa_format == LFA_COMPETITIVE ? !t_line : t_line;
    result.verdict = positive ? STRIP_POSITIVE : STRIP_NEGATIVE;
    return result;
}
