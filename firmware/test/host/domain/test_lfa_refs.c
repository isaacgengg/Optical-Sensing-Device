#include "unity.h"
#include <stdio.h>
#include <string.h>
#include "strip_analysis.h"
#include "lfa_refs.h"
#include "../data/paper_lfa_2026_11_23.h"

void setUp(void) {}
void tearDown(void) {}

static const strip_config_t CFG = {
    .mode = STRIP_MODE_LATERAL_FLOW,
    .test_ref = &LFA_TEST_REF, .control_ref = &LFA_CONTROL_REF,
    .lfa_format = LFA_COMPETITIVE,
};

static void replay_batch_range(unsigned first, unsigned last, size_t expected_count)
{
    size_t replayed = 0;
    for (size_t i = 0; i < sizeof(LFA_BENCH_READINGS) / sizeof(LFA_BENCH_READINGS[0]); i++) {
        const lfa_bench_reading_t *capture = &LFA_BENCH_READINGS[i];
        if (capture->batch < first || capture->batch > last) continue;
        char source[64];
        snprintf(source, sizeof(source), "batch%u uptime=%lu",
                 capture->batch, (unsigned long)capture->uptime_ms);
        channel_reading_t t = {.count = 14}, c = {.count = 14};
        memcpy(t.values, capture->test_raw, sizeof(capture->test_raw));
        memcpy(c.values, capture->control_raw, sizeof(capture->control_raw));
        // Each logged vector is already an average of five hardware samples.
        // Replay that vector once; the individual hardware samples are unknown.
        strip_result_t r = strip_analyze_lateral_flow(&t, &c, 1, &CFG);
        TEST_ASSERT_TRUE_MESSAGE(r.lfa_calibration_valid, source);
        TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.000001f, capture->test_contrast, r.score, source);
        TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.000001f, capture->control_contrast, r.control_score, source);
        TEST_ASSERT_EQUAL_MESSAGE(capture->test_line, r.score >= LFA_TEST_REF.threshold, source);
        TEST_ASSERT_EQUAL_MESSAGE(capture->control_line, r.control_score >= LFA_CONTROL_REF.threshold, source);
        strip_verdict_t expected = !capture->control_line ? STRIP_INVALID
                                   : capture->test_line ? STRIP_NEGATIVE : STRIP_POSITIVE;
        TEST_ASSERT_EQUAL_MESSAGE(expected, r.verdict, source);
        replayed++;
    }
    TEST_ASSERT_EQUAL_size_t(expected_count, replayed);
}

static void test_fit_replays_all_34_labeled_captures(void)
{
    replay_batch_range(11, 15, 34);
}

static void test_frozen_fit_replays_six_new_stationary_control_captures(void)
{
    replay_batch_range(16, 16, 6);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_fit_replays_all_34_labeled_captures);
    RUN_TEST(test_frozen_fit_replays_six_new_stationary_control_captures);
    return UNITY_END();
}
