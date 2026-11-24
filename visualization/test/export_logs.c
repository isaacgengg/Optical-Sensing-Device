// Synthetic samples through the production analysis and log sink.
#include <stdio.h>
#include "strip_analysis.h"
#include "log_sink.h"
#include "color_refs.h"

static void emit(const char *name, const strip_result_t *r)
{
    printf("CASE %s\nI (0) main: source=MOCK synthetic host fixture\n", name);
    result_sink_t *sink = log_sink_create();
    sink->publish(sink->ctx, r);
}

int main(void)
{
    strip_config_t cfg = {.mode = STRIP_MODE_COLORIMETRIC, .white = &COLOR_WHITE,
        .refs = COLOR_REFS, .ref_count = sizeof(COLOR_REFS) / sizeof(COLOR_REFS[0]),
        .max_distance = COLOR_MAX_DISTANCE, .min_color_margin = COLOR_MIN_MARGIN};
    for (size_t i = 0; i < cfg.ref_count; i++) {
        for (int half = 0; half <= 1; half++) {
            channel_reading_t sample = {.count = COLOR_WHITE.count};
            for (size_t c = 0; c < sample.count; c++)
                sample.values[c] = COLOR_WHITE.values[c] * COLOR_REFS[i].reflectance[c] * (half ? 0.5f : 1.0f);
            strip_result_t r = strip_analyze(&sample, 1, &cfg);
            if (r.verdict != COLOR_REFS[i].verdict || r.score > 0.00001f) return 1;
            char name[64];
            snprintf(name, sizeof(name), "%s_%s", COLOR_REFS[i].color, half ? "half" : "full");
            emit(name, &r);
        }
    }
    channel_reading_t sample = {.count = COLOR_WHITE.count};
    float am = 0, bm = 0;
    for (size_t c = 0; c < sample.count; c++) {
        am += COLOR_REFS[2].reflectance[c];
        bm += COLOR_REFS[4].reflectance[c];
    }
    am /= sample.count; bm /= sample.count;
    for (size_t c = 0; c < sample.count; c++)
        sample.values[c] = COLOR_WHITE.values[c] *
            (COLOR_REFS[2].reflectance[c] / am + COLOR_REFS[4].reflectance[c] / bm) / 2;
    strip_result_t ambiguous = strip_analyze(&sample, 1, &cfg);
    if (ambiguous.verdict != STRIP_INVALID || ambiguous.color_match.status != COLOR_MATCH_AMBIGUOUS) return 1;
    emit("ambiguous_color", &ambiguous);
    sample = (channel_reading_t){.count = COLOR_WHITE.count};
    strip_result_t r = strip_analyze(&sample, 1, &cfg);
    emit("dark_color", &r);
    for (size_t c = 0; c < sample.count; c++) sample.values[c] = COLOR_WHITE.values[c];
    sample.values[0] *= 10;
    r = strip_analyze(&sample, 1, &cfg);
    if (r.verdict != STRIP_INVALID) return 1;
    emit("unmatched_color", &r);
    cfg = (strip_config_t){.mode = STRIP_MODE_LATERAL_FLOW};
    r = strip_analyze_lateral_flow(&sample, &sample, 1, &cfg);
    emit("uncalibrated_lfa", &r);
    const lfa_zone_ref_t ref = {.dark = {.count = 14},
        .blank = {.values = {[4] = 2000}, .count = 14}, .channel_mask = 1u << 4, .threshold = 0.3f};
    cfg.test_ref = cfg.control_ref = &ref;
    cfg.lfa_format = LFA_COMPETITIVE;
    sample = (channel_reading_t){.count = 14, .values = {[4] = 1000}};
    r = strip_analyze_lateral_flow(&sample, &sample, 1, &cfg);
    emit("calibrated_lfa", &r);
    printf("CASE lfa_dark\nI (0) main: source=MOCK synthetic host fixture\n");
    const lfa_dark_capture_t dark = {
        .test_raw = {.count = 14, .values = {0.2f, 0.4f}},
        .control_raw = {.count = 14, .values = {0.6f, 0.8f}},
        .sample_count = 5, .test_gain = 128, .control_gain = 64,
        .test_integration_ms = 100, .control_integration_ms = 100,
    };
    if (log_sink_lfa_dark(&dark) != ESP_OK) return 1;
    return 0;
}
