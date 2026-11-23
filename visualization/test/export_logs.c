// Synthetic samples through the unchanged analysis AND unchanged log sink.
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
        .max_distance = COLOR_MAX_DISTANCE};
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
    return 0;
}
