// Colorimetric: reflectance against a white reference, matched to the nearest
// known pad color (firmware/main/color_refs.h). LFA uses separate zone refs.
#include <math.h>
#include <stdbool.h>
#include "strip_analysis.h"

channel_reading_t strip_average(const channel_reading_t *samples, size_t n)
{
    channel_reading_t avg = {0};
    if (samples == NULL || n == 0) {
        return avg;
    }

    avg.count = samples[0].count;
    if (avg.count == 0 || avg.count > DC_MAX_CHANNELS) return (channel_reading_t){0};
    for (size_t i = 0; i < n; i++) {
        if (samples[i].count != avg.count) return (channel_reading_t){0};
        for (size_t c = 0; c < avg.count; c++) {
            if (!isfinite(samples[i].values[c])) return (channel_reading_t){0};
            avg.values[c] += samples[i].values[c];
        }
    }
    for (size_t c = 0; c < avg.count; c++) {
        avg.values[c] /= (float)n;
    }
    return avg;
}

// Normalize each zone against its own blank membrane and dark measurement.
static bool line_contrast(const channel_reading_t *raw, const lfa_zone_ref_t *ref,
                          float *score)
{
    if (ref == NULL || raw->count == 0 || raw->count > DC_MAX_CHANNELS ||
        ref->blank.count != raw->count || ref->dark.count != raw->count ||
        ref->channel_mask == 0 || (ref->channel_mask & ~0x07FFu) != 0 ||
        (ref->channel_mask >> raw->count) != 0 ||
        !isfinite(ref->threshold) || ref->threshold <= 0 || ref->threshold > 1) return false;
    float sum = 0;
    size_t count = 0;
    for (size_t i = 0; i < raw->count; i++) {
        if (!(ref->channel_mask & (1u << i))) continue;
        float blank = ref->blank.values[i], dark = ref->dark.values[i];
        if (!isfinite(raw->values[i]) || !isfinite(blank) || !isfinite(dark) ||
            dark < 0 || blank <= dark || raw->values[i] < dark) return false;
        sum += 1.0f - (raw->values[i] - dark) / (blank - dark);
        count++;
    }
    *score = sum / (float)count;
    return isfinite(*score);
}

strip_result_t strip_analyze(const channel_reading_t *samples, size_t n,
                             const strip_config_t *cfg)
{
    strip_result_t result = {.verdict = STRIP_INVALID, .sample_count = n};
    if (samples == NULL || cfg == NULL || n == 0) {
        return result;
    }
    result.mode = cfg->mode;
    result.raw = strip_average(samples, n);

    const channel_reading_t *white = cfg->white;
    if (white == NULL || cfg->refs == NULL || cfg->ref_count == 0 ||
        result.raw.count == 0 || white->count != result.raw.count) {
        return result;
    }
    size_t count = result.raw.count;
    for (size_t c = 0; c < count; c++) {
        if (white->values[c] <= 0.0f) {
            return result;  // dead or uncalibrated channel: can't divide by it
        }
        result.reflectance.values[c] = result.raw.values[c] / white->values[c];
    }
    result.reflectance.count = count;

    // ponytail: nearest neighbour on all channels, equally weighted. Weight or
    // drop channels (FD is flicker, VIS is broadband) if real data says so.
    float best = INFINITY;
    const color_ref_t *best_ref = NULL;
    for (size_t r = 0; r < cfg->ref_count; r++) {
        float sum = 0.0f;
        for (size_t c = 0; c < count; c++) {
            float d = result.reflectance.values[c] - cfg->refs[r].reflectance[c];
            sum += d * d;
        }
        float rms = sqrtf(sum / (float)count);
        if (rms < best) {
            best = rms;
            best_ref = &cfg->refs[r];
        }
    }
    result.score = best;
    if (best <= cfg->max_distance) {
        result.match = best_ref->name;
        result.color = best_ref->color;
        result.verdict = best_ref->verdict;
    }
    return result;
}

strip_result_t strip_analyze_lateral_flow(const channel_reading_t *t_samples,
                                          const channel_reading_t *c_samples,
                                          size_t n, const strip_config_t *cfg)
{
    strip_result_t result = {
        .mode = STRIP_MODE_LATERAL_FLOW, .verdict = STRIP_INVALID, .sample_count = n,
        .score = 0, .control_score = 0};
    if (t_samples == NULL || c_samples == NULL || cfg == NULL || n == 0) {
        return result;
    }
    result.raw = strip_average(t_samples, n);
    result.control_raw = strip_average(c_samples, n);
    if (!line_contrast(&result.raw, cfg->test_ref, &result.score) ||
        !line_contrast(&result.control_raw, cfg->control_ref, &result.control_score)) {
        result.score = result.control_score = 0;
        return result;
    }

    result.lfa_calibration_valid = true;

    // No C line: the sample didn't flow, so the T line means nothing.
    if (result.control_score < cfg->control_ref->threshold) {
        return result;
    }

    bool t_line = result.score >= cfg->test_ref->threshold;
    bool positive = cfg->lfa_format == LFA_COMPETITIVE ? !t_line : t_line;
    result.verdict = positive ? STRIP_POSITIVE : STRIP_NEGATIVE;
    return result;
}
