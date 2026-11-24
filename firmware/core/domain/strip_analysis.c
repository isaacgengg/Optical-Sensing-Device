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
        result.raw.count == 0 || white->count != result.raw.count ||
        !isfinite(cfg->min_color_margin) || cfg->min_color_margin < 0.0f) {
        return result;
    }
    size_t count = result.raw.count;
    for (size_t c = 0; c < count; c++) {
        if (!isfinite(white->values[c]) || white->values[c] <= 0.0f) {
            return result;  // dead or uncalibrated channel: can't divide by it
        }
        result.reflectance.values[c] = result.raw.values[c] / white->values[c];
    }
    result.reflectance.count = count;

    // Shape matching: divide reading and ref by their own channel mean, so a
    // uniformly brighter or dimmer pad still matches. Changes in angle or
    // coverage can change shape and are not corrected by this normalization.
    // Cost: white and an even grey look the same.
    // Nearest neighbour on all channels, equally weighted. Weight or
    // drop channels (FD is flicker, VIS is broadband) if real data says so.
    float mean = 0.0f;
    for (size_t c = 0; c < count; c++) mean += result.reflectance.values[c];
    mean /= (float)count;
    if (!isfinite(mean) || !(mean > 0.0f)) return result;

    float best = INFINITY;
    float second = INFINITY;
    const color_ref_t *best_ref = NULL;
    const color_ref_t *second_ref = NULL;
    for (size_t r = 0; r < cfg->ref_count; r++) {
        float ref_mean = 0.0f;
        for (size_t c = 0; c < count; c++) ref_mean += cfg->refs[r].reflectance[c];
        ref_mean /= (float)count;
        if (!isfinite(ref_mean) || !(ref_mean > 0.0f)) return result;
        float sum = 0.0f;
        for (size_t c = 0; c < count; c++) {
            float d = result.reflectance.values[c] / mean -
                      cfg->refs[r].reflectance[c] / ref_mean;
            sum += d * d;
        }
        float rms = sqrtf(sum / (float)count);
        if (!isfinite(rms)) return result;
        if (rms < best) {
            second = best;
            second_ref = best_ref;
            best = rms;
            best_ref = &cfg->refs[r];
        } else if (rms < second) {
            second = rms;
            second_ref = &cfg->refs[r];
        }
    }
    result.score = best;
    if (best_ref == NULL) return result;
    // Select the nearest color first, then apply only its cutoff. A rejected
    // nearest color must not fall through to a farther, more permissive one.
    float max_distance = best_ref->max_distance == 0.0f
                             ? cfg->max_distance : best_ref->max_distance;
    if (!isfinite(max_distance) || max_distance < 0.0f) return result;
    result.color_match = (color_match_info_t){
        .nearest = best_ref->color ? best_ref->color : best_ref->name,
        .runner_up = second_ref ? (second_ref->color ? second_ref->color : second_ref->name) : NULL,
        .runner_up_score = second,
        .margin = second - best,
        .max_distance = max_distance,
        .min_margin = cfg->min_color_margin,
    };
    if (best > max_distance) {
        result.color_match.status = COLOR_MATCH_TOO_FAR;
        return result;
    }
    if (second_ref && second - best < cfg->min_color_margin) {
        result.color_match.status = COLOR_MATCH_AMBIGUOUS;
        return result;
    }
    result.color_match.status = COLOR_MATCH_ACCEPTED;
    result.match = best_ref->name;
    result.color = best_ref->color;
    result.verdict = best_ref->verdict;
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
