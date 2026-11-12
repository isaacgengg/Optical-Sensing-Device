#include "unity.h"
#include <math.h>
#include "strip_analysis.h"

static channel_reading_t uniform(float v, size_t count)
{
    channel_reading_t r = {.count = count};
    for (size_t i = 0; i < count; i++) {
        r.values[i] = v;
    }
    return r;
}

void setUp(void) {}
void tearDown(void) {}

static void test_average_per_channel(void)
{
    channel_reading_t s[2] = {
        {.values = {1, 2, 3}, .count = 3},
        {.values = {3, 6, 9}, .count = 3},
    };
    channel_reading_t avg = strip_average(s, 2);
    TEST_ASSERT_EQUAL_size_t(3, avg.count);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, avg.values[0]);
    TEST_ASSERT_EQUAL_FLOAT(4.0f, avg.values[1]);
    TEST_ASSERT_EQUAL_FLOAT(6.0f, avg.values[2]);
}

static void test_average_zero_samples(void)
{
    channel_reading_t s[1] = {uniform(5, 4)};
    TEST_ASSERT_EQUAL_size_t(0, strip_average(s, 0).count);
}

// --- colorimetric ---

static const channel_reading_t WHITE = {.values = {1000, 2000}, .count = 2};
static const color_ref_t REFS[] = {
    {.name = "clear", .verdict = STRIP_NEGATIVE, .reflectance = {0.9f, 0.9f}},
    {.name = "blue", .color = "blue", .verdict = STRIP_POSITIVE, .reflectance = {0.8f, 0.2f}},
};
static const strip_config_t CFG = {
    .mode = STRIP_MODE_COLORIMETRIC, .white = &WHITE, .refs = REFS, .ref_count = 2,
    .max_distance = 0.1f};

static strip_result_t color(float ch0, float ch1, const strip_config_t *cfg)
{
    channel_reading_t s[1] = {{.values = {ch0, ch1}, .count = 2}};
    return strip_analyze(s, 1, cfg);
}

static void test_color_reflectance_is_raw_over_white(void)
{
    strip_result_t r = color(500, 500, &CFG);
    TEST_ASSERT_EQUAL_size_t(2, r.reflectance.count);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, r.reflectance.values[0]);
    TEST_ASSERT_EQUAL_FLOAT(0.25f, r.reflectance.values[1]);
    TEST_ASSERT_EQUAL_FLOAT(500.0f, r.raw.values[1]);
}

static void test_color_matches_nearest_reference(void)
{
    strip_result_t r = color(800, 400, &CFG);  // refl 0.8, 0.2: exactly "blue"
    TEST_ASSERT_EQUAL(STRIP_POSITIVE, r.verdict);
    TEST_ASSERT_EQUAL_STRING("blue", r.match);
    TEST_ASSERT_EQUAL_STRING("blue", r.color);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, r.score);
    TEST_ASSERT_EQUAL(STRIP_MODE_COLORIMETRIC, r.mode);

    r = color(880, 1840, &CFG);  // refl 0.88, 0.92: near "clear"
    TEST_ASSERT_EQUAL(STRIP_NEGATIVE, r.verdict);
    TEST_ASSERT_EQUAL_STRING("clear", r.match);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.02f, r.score);  // RMS of (0.02, 0.02)
}

static void test_color_too_far_from_every_reference_is_invalid(void)
{
    strip_result_t r = color(100, 1800, &CFG);  // refl 0.1, 0.9: no such pad
    TEST_ASSERT_EQUAL(STRIP_INVALID, r.verdict);
    TEST_ASSERT_NULL(r.match);
    TEST_ASSERT_NULL(r.color);
    TEST_ASSERT_TRUE(r.score > CFG.max_distance);
}

static void test_color_distance_boundary(void)
{
    // Values exact in binary: refl 0.25 vs ref 0.75 gives RMS exactly 0.5.
    static const color_ref_t one[] = {{.name = "grey", .verdict = STRIP_NEGATIVE, .reflectance = {0.75f, 0.75f}}};
    strip_config_t cfg = CFG;
    cfg.refs = one;
    cfg.ref_count = 1;
    cfg.max_distance = 0.5f;
    TEST_ASSERT_EQUAL(STRIP_NEGATIVE, color(250, 500, &cfg).verdict);
    cfg.max_distance = 0.49f;
    TEST_ASSERT_EQUAL(STRIP_INVALID, color(250, 500, &cfg).verdict);
}

static void test_color_bad_config_is_invalid(void)
{
    channel_reading_t s[1] = {{.values = {800, 400}, .count = 2}};
    TEST_ASSERT_EQUAL(STRIP_INVALID, strip_analyze(s, 0, &CFG).verdict);
    TEST_ASSERT_EQUAL(STRIP_INVALID, strip_analyze(NULL, 1, &CFG).verdict);
    TEST_ASSERT_EQUAL(STRIP_INVALID, strip_analyze(s, 1, NULL).verdict);

    strip_config_t cfg = CFG;
    cfg.white = NULL;
    TEST_ASSERT_EQUAL(STRIP_INVALID, strip_analyze(s, 1, &cfg).verdict);

    cfg = CFG;
    cfg.ref_count = 0;
    TEST_ASSERT_EQUAL(STRIP_INVALID, strip_analyze(s, 1, &cfg).verdict);

    channel_reading_t white3 = {.values = {1, 1, 1}, .count = 3};  // channel count mismatch
    cfg = CFG;
    cfg.white = &white3;
    TEST_ASSERT_EQUAL(STRIP_INVALID, strip_analyze(s, 1, &cfg).verdict);

    channel_reading_t dead = {.values = {1000, 0}, .count = 2};  // can't divide by 0
    cfg.white = &dead;
    strip_result_t r = strip_analyze(s, 1, &cfg);
    TEST_ASSERT_EQUAL(STRIP_INVALID, r.verdict);
    TEST_ASSERT_NULL(r.match);
}

// --- lateral flow ---
static const lfa_zone_ref_t T_REF = {
    .dark = {.values = {10, 20}, .count = 2},
    .blank = {.values = {110, 220}, .count = 2},
    .channel_mask = 3, .threshold = 0.5f,
};
static const lfa_zone_ref_t C_REF = {
    .dark = {.values = {100, 200}, .count = 2},
    .blank = {.values = {1100, 2200}, .count = 2},
    .channel_mask = 3, .threshold = 0.25f,
};
static strip_result_t lfa(float t, float c, lfa_format_t format)
{
    channel_reading_t ts[] = {{.values = {10 + 100*(1-t), 20 + 200*(1-t)}, .count = 2}};
    channel_reading_t cs[] = {{.values = {100 + 1000*(1-c), 200 + 2000*(1-c)}, .count = 2}};
    strip_config_t cfg = {.mode = STRIP_MODE_LATERAL_FLOW, .test_ref = &T_REF,
                         .control_ref = &C_REF, .lfa_format = format};
    return strip_analyze_lateral_flow(ts, cs, 1, &cfg);
}
static void test_lfa_independent_normalization_and_rules(void)
{
    strip_result_t r = lfa(0.5f, 0.25f, LFA_COMPETITIVE);
    TEST_ASSERT_EQUAL(STRIP_NEGATIVE, r.verdict);
    TEST_ASSERT_TRUE(r.lfa_calibration_valid);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, r.score);
    TEST_ASSERT_EQUAL_FLOAT(0.25f, r.control_score);
    TEST_ASSERT_EQUAL_FLOAT(850, r.control_raw.values[0]);
    TEST_ASSERT_EQUAL(STRIP_POSITIVE, lfa(0.49f, 0.3f, LFA_COMPETITIVE).verdict);
    TEST_ASSERT_EQUAL(STRIP_POSITIVE, lfa(0.5f, 0.3f, LFA_STANDARD).verdict);
    TEST_ASSERT_EQUAL(STRIP_NEGATIVE, lfa(0.49f, 0.3f, LFA_STANDARD).verdict);
    TEST_ASSERT_EQUAL(STRIP_INVALID, lfa(0.8f, 0.24f, LFA_COMPETITIVE).verdict);
    TEST_ASSERT_EQUAL(STRIP_INVALID, lfa(0, 0.24f, LFA_STANDARD).verdict);
}
static void test_lfa_invalid_calibration_and_samples(void)
{
    channel_reading_t s[] = {{.values = {60, 120}, .count = 2}};
    lfa_zone_ref_t ref = T_REF;
    strip_config_t cfg = {.test_ref = &ref, .control_ref = &T_REF};
    cfg.test_ref = NULL;
    strip_result_t r = strip_analyze_lateral_flow(s, s, 1, &cfg);
    TEST_ASSERT_EQUAL(STRIP_INVALID, r.verdict);
    TEST_ASSERT_FALSE(r.lfa_calibration_valid);
    TEST_ASSERT_EQUAL_size_t(2, r.raw.count); // raw data still available for calibration
    cfg.test_ref = &ref;
    ref.blank.values[0] = ref.dark.values[0];
    TEST_ASSERT_FALSE(strip_analyze_lateral_flow(s, s, 1, &cfg).lfa_calibration_valid);
    ref = T_REF; ref.channel_mask = 1u << 13; // never use flicker in line contrast
    TEST_ASSERT_FALSE(strip_analyze_lateral_flow(s, s, 1, &cfg).lfa_calibration_valid);
    ref = T_REF; ref.threshold = NAN;
    TEST_ASSERT_FALSE(strip_analyze_lateral_flow(s, s, 1, &cfg).lfa_calibration_valid);
    ref = T_REF; ref.dark.count = 3;
    TEST_ASSERT_FALSE(strip_analyze_lateral_flow(s, s, 1, &cfg).lfa_calibration_valid);
    ref = T_REF; s[0].values[0] = 0; // below dark reference
    TEST_ASSERT_FALSE(strip_analyze_lateral_flow(s, s, 1, &cfg).lfa_calibration_valid);
    s[0].values[0] = NAN;
    TEST_ASSERT_EQUAL(STRIP_INVALID, strip_analyze_lateral_flow(s, s, 1, &cfg).verdict);
    TEST_ASSERT_EQUAL(STRIP_INVALID, strip_analyze_lateral_flow(NULL, s, 1, &cfg).verdict);
    TEST_ASSERT_EQUAL(STRIP_INVALID, strip_analyze_lateral_flow(s, NULL, 1, &cfg).verdict);
    TEST_ASSERT_EQUAL(STRIP_INVALID, strip_analyze_lateral_flow(s, s, 0, &cfg).verdict);
    TEST_ASSERT_EQUAL(STRIP_INVALID, strip_analyze_lateral_flow(s, s, 1, NULL).verdict);
}
static void test_average_rejects_inconsistent_channel_counts(void)
{
    channel_reading_t s[] = {{.count = 2}, {.count = 1}};
    TEST_ASSERT_EQUAL_size_t(0, strip_average(s, 2).count);
    s[0].count = DC_MAX_CHANNELS + 1;
    TEST_ASSERT_EQUAL_size_t(0, strip_average(s, 1).count);
}
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_average_per_channel);
    RUN_TEST(test_average_zero_samples);
    RUN_TEST(test_average_rejects_inconsistent_channel_counts);
    RUN_TEST(test_color_reflectance_is_raw_over_white);
    RUN_TEST(test_color_matches_nearest_reference);
    RUN_TEST(test_color_too_far_from_every_reference_is_invalid);
    RUN_TEST(test_color_distance_boundary);
    RUN_TEST(test_color_bad_config_is_invalid);
    RUN_TEST(test_lfa_independent_normalization_and_rules);
    RUN_TEST(test_lfa_invalid_calibration_and_samples);
    return UNITY_END();
}
