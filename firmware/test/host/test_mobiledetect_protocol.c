#include <string.h>
#include "unity.h"
#include "mobiledetect_protocol.h"

void setUp(void) {}
void tearDown(void) {}

static void test_v0_layout(void)
{
    strip_result_t r = {.verdict = STRIP_POSITIVE, .score = 1.0f, .sample_count = 0x0102};
    uint8_t buf[16];
    TEST_ASSERT_EQUAL_size_t(8, mdp_encode_result(&r, buf, sizeof(buf)));

    // 1.0f is 0x3F800000.
    const uint8_t expected[8] = {MDP_VERSION, STRIP_POSITIVE, 0x00, 0x00, 0x80, 0x3F, 0x02, 0x01};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, buf, 8);
}

static void test_buffer_too_small(void)
{
    strip_result_t r = {0};
    uint8_t buf[7];
    TEST_ASSERT_EQUAL_size_t(0, mdp_encode_result(&r, buf, sizeof(buf)));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_v0_layout);
    RUN_TEST(test_buffer_too_small);
    return UNITY_END();
}
