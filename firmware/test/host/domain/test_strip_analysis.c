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
    return UNITY_END();
}
