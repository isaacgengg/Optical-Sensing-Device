#include "unity.h"
#include "strip_analysis.h"
#include "lfa_refs.h"

void setUp(void) {}
void tearDown(void) {}

static void test_profile_matches_acquisition_configuration(void)
{
    const strip_config_t cfg = {
        .mode = STRIP_MODE_LATERAL_FLOW,
        .test_ref = &LFA_TEST_REF, .control_ref = &LFA_CONTROL_REF,
        .lfa_format = LFA_COMPETITIVE,
    };
    const channel_reading_t sample = {.values = {[4] = 500}, .count = 14};
    strip_result_t result = strip_analyze_lateral_flow(&sample, &sample, 1, &cfg);
#if CONFIG_DC_SENSOR_MOCK
    TEST_ASSERT_EQUAL_STRING("synthetic-mock", LFA_PROFILE_ID);
    TEST_ASSERT_TRUE(result.lfa_calibration_valid);
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, 0.75f, result.score);
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, 0.5f, result.control_score);
    TEST_ASSERT_EQUAL(STRIP_NEGATIVE, result.verdict);
#else
    TEST_ASSERT_EQUAL_STRING("uncalibrated", LFA_PROFILE_ID);
    TEST_ASSERT_FALSE(result.lfa_calibration_valid);
    TEST_ASSERT_EQUAL(STRIP_INVALID, result.verdict);
    TEST_ASSERT_EQUAL_FLOAT(0, result.score);
    TEST_ASSERT_EQUAL_FLOAT(0, result.control_score);
#endif
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_profile_matches_acquisition_configuration);
    return UNITY_END();
}
