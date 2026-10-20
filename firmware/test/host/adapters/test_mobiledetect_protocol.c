#include <string.h>
#include "unity.h"
#include "mobiledetect_protocol.h"

void setUp(void) {}
void tearDown(void) {}

static void test_v1_layout(void)
{
    strip_result_t r = {
        .mode = STRIP_MODE_LATERAL_FLOW, .verdict = STRIP_POSITIVE,
        .score = 1.0f, .control_score = -2.0f, .sample_count = 0x0102};
    uint8_t buf[16];
    TEST_ASSERT_EQUAL_size_t(13, mdp_encode_result(&r, buf, sizeof(buf)));

    // 1.0f is 0x3F800000, -2.0f is 0xC0000000.
    const uint8_t expected[13] = {
        1, STRIP_MODE_LATERAL_FLOW, STRIP_POSITIVE,
        0x00, 0x00, 0x80, 0x3F,
        0x00, 0x00, 0x00, 0xC0,
        0x02, 0x01};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, buf, 13);
}

static void test_colorimetric_mode_and_sample_clamp(void)
{
    strip_result_t r = {.mode = STRIP_MODE_COLORIMETRIC, .verdict = STRIP_INVALID, .sample_count = 70000};
    uint8_t buf[MDP_RESULT_LEN];
    TEST_ASSERT_EQUAL_size_t(MDP_RESULT_LEN, mdp_encode_result(&r, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_UINT8(1, buf[1]);
    TEST_ASSERT_EQUAL_UINT8(2, buf[2]);
    TEST_ASSERT_EQUAL_UINT8(0xFF, buf[11]);  // clamped to 65535
    TEST_ASSERT_EQUAL_UINT8(0xFF, buf[12]);
}

static void test_buffer_too_small(void)
{
    strip_result_t r = {0};
    uint8_t buf[MDP_RESULT_LEN - 1];
    TEST_ASSERT_EQUAL_size_t(0, mdp_encode_result(&r, buf, sizeof(buf)));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_v1_layout);
    RUN_TEST(test_colorimetric_mode_and_sample_clamp);
    RUN_TEST(test_buffer_too_small);
    return UNITY_END();
}
