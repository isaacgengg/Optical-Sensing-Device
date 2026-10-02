#include "unity.h"
#include "strip_analysis.h"

static const strip_config_t CFG = {.positive_threshold = 100.0f, .min_signal = 10.0f};

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

static void test_zero_samples_invalid(void)
{
    channel_reading_t s[1] = {uniform(500, 4)};
    TEST_ASSERT_EQUAL(STRIP_INVALID, strip_analyze(s, 0, &CFG).verdict);
}

static void test_low_signal_invalid(void)
{
    channel_reading_t s[1] = {uniform(9.9f, 4)};
    strip_result_t r = strip_analyze(s, 1, &CFG);
    TEST_ASSERT_EQUAL(STRIP_INVALID, r.verdict);
    TEST_ASSERT_EQUAL_FLOAT(9.9f, r.score);
}

static void test_threshold_boundaries(void)
{
    channel_reading_t s[1];

    s[0] = uniform(10.0f, 4);  // exactly min_signal
    TEST_ASSERT_EQUAL(STRIP_NEGATIVE, strip_analyze(s, 1, &CFG).verdict);

    s[0] = uniform(99.9f, 4);
    TEST_ASSERT_EQUAL(STRIP_NEGATIVE, strip_analyze(s, 1, &CFG).verdict);

    s[0] = uniform(100.0f, 4);  // exactly positive_threshold
    TEST_ASSERT_EQUAL(STRIP_POSITIVE, strip_analyze(s, 1, &CFG).verdict);
}

static void test_score_is_mean_across_channels(void)
{
    channel_reading_t s[1] = {{.values = {50, 150}, .count = 2}};
    strip_result_t r = strip_analyze(s, 1, &CFG);
    TEST_ASSERT_EQUAL_FLOAT(100.0f, r.score);
    TEST_ASSERT_EQUAL_size_t(1, r.sample_count);
}

static void test_result_carries_mode(void)
{
    channel_reading_t s[1] = {uniform(50, 4)};
    strip_config_t cfg = CFG;
    cfg.mode = STRIP_MODE_COLORIMETRIC;
    TEST_ASSERT_EQUAL(STRIP_MODE_COLORIMETRIC, strip_analyze(s, 1, &cfg).mode);
}

// --- lateral flow ---

static const strip_config_t LFA = {.t_threshold = 100.0f, .c_threshold = 50.0f};

static strip_verdict_t lfa(float t, float c, lfa_format_t format)
{
    channel_reading_t ts[1] = {uniform(t, 4)};
    channel_reading_t cs[1] = {uniform(c, 4)};
    strip_config_t cfg = LFA;
    cfg.lfa_format = format;
    return strip_analyze_lateral_flow(ts, cs, 1, &cfg).verdict;
}

static void test_lfa_no_control_line_is_invalid(void)
{
    TEST_ASSERT_EQUAL(STRIP_INVALID, lfa(500, 49.9f, LFA_STANDARD));
    TEST_ASSERT_EQUAL(STRIP_INVALID, lfa(0, 49.9f, LFA_STANDARD));
    TEST_ASSERT_EQUAL(STRIP_INVALID, lfa(500, 49.9f, LFA_COMPETITIVE));
    TEST_ASSERT_EQUAL(STRIP_INVALID, lfa(0, 49.9f, LFA_COMPETITIVE));
}

static void test_lfa_standard_t_line_means_positive(void)
{
    TEST_ASSERT_EQUAL(STRIP_POSITIVE, lfa(100, 50, LFA_STANDARD));  // both exactly at threshold
    TEST_ASSERT_EQUAL(STRIP_NEGATIVE, lfa(99.9f, 50, LFA_STANDARD));
}

static void test_lfa_competitive_t_line_means_negative(void)
{
    TEST_ASSERT_EQUAL(STRIP_NEGATIVE, lfa(100, 50, LFA_COMPETITIVE));
    TEST_ASSERT_EQUAL(STRIP_POSITIVE, lfa(99.9f, 50, LFA_COMPETITIVE));
}

static void test_lfa_scores_and_mode(void)
{
    channel_reading_t ts[2] = {uniform(100, 4), uniform(300, 4)};
    channel_reading_t cs[2] = {uniform(60, 4), uniform(80, 4)};
    strip_result_t r = strip_analyze_lateral_flow(ts, cs, 2, &LFA);
    TEST_ASSERT_EQUAL(STRIP_MODE_LATERAL_FLOW, r.mode);
    TEST_ASSERT_EQUAL_FLOAT(200.0f, r.score);
    TEST_ASSERT_EQUAL_FLOAT(70.0f, r.control_score);
    TEST_ASSERT_EQUAL_size_t(2, r.sample_count);
}

static void test_lfa_bad_input_is_invalid(void)
{
    channel_reading_t s[1] = {uniform(500, 4)};
    TEST_ASSERT_EQUAL(STRIP_INVALID, strip_analyze_lateral_flow(s, s, 0, &LFA).verdict);
    TEST_ASSERT_EQUAL(STRIP_INVALID, strip_analyze_lateral_flow(NULL, s, 1, &LFA).verdict);
    TEST_ASSERT_EQUAL(STRIP_INVALID, strip_analyze_lateral_flow(s, NULL, 1, &LFA).verdict);
    TEST_ASSERT_EQUAL(STRIP_INVALID, strip_analyze_lateral_flow(s, s, 1, NULL).verdict);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_average_per_channel);
    RUN_TEST(test_average_zero_samples);
    RUN_TEST(test_zero_samples_invalid);
    RUN_TEST(test_low_signal_invalid);
    RUN_TEST(test_threshold_boundaries);
    RUN_TEST(test_score_is_mean_across_channels);
    RUN_TEST(test_result_carries_mode);
    RUN_TEST(test_lfa_no_control_line_is_invalid);
    RUN_TEST(test_lfa_standard_t_line_means_positive);
    RUN_TEST(test_lfa_competitive_t_line_means_negative);
    RUN_TEST(test_lfa_scores_and_mode);
    RUN_TEST(test_lfa_bad_input_is_invalid);
    return UNITY_END();
}
