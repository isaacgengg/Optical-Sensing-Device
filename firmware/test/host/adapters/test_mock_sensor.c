#include "unity.h"
#include "mock_sensor.h"

void setUp(void) {}
void tearDown(void) {}

static void test_replays_in_order_and_wraps(void)
{
    const channel_reading_t table[3] = {
        {.values = {1}, .count = 1},
        {.values = {2}, .count = 1},
        {.values = {3}, .count = 1},
    };
    optical_sensor_t *s = mock_sensor_create(table, 3);
    TEST_ASSERT_EQUAL(ESP_OK, s->init(s->ctx));
    TEST_ASSERT_EQUAL_size_t(1, s->channel_count);

    const float expected[] = {1, 2, 3, 1, 2};
    for (size_t i = 0; i < 5; i++) {
        channel_reading_t r;
        TEST_ASSERT_EQUAL(ESP_OK, s->read(s->ctx, &r));
        TEST_ASSERT_EQUAL_FLOAT(expected[i], r.values[0]);
    }
}

static void test_default_table_is_8_channels(void)
{
    TEST_ASSERT_TRUE(mock_default_table_len > 0);
    optical_sensor_t *s = mock_sensor_create(mock_default_table, mock_default_table_len);
    TEST_ASSERT_EQUAL_size_t(8, s->channel_count);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_replays_in_order_and_wraps);
    RUN_TEST(test_default_table_is_8_channels);
    return UNITY_END();
}
