#include "unity.h"
#include "measurement.h"
#include "mock_sensor.h"
#include "fake_illuminator.h"
#include "fake_timebase.h"
#include "fake_sink.h"

static measurement_deps_t deps;
static measurement_config_t cfg;

void setUp(void)
{
    deps = (measurement_deps_t){
        .sensor = mock_sensor_create(mock_default_table, mock_default_table_len),
        .light = fake_illuminator_create(),
        .time = fake_timebase_create(),
        .sink = fake_sink_create(),
    };
    cfg = (measurement_config_t){
        .integration_ms = 100,
        .led_intensity_pct = 80,
        .led_settle_ms = 50,
        .sample_count = 3,
        .analysis = {.positive_threshold = 1000.0f, .min_signal = 10.0f},
    };
    TEST_ASSERT_EQUAL(ESP_OK, measurement_init(&deps));
}

void tearDown(void) {}

// Tiny sensor fake that fails on the second read.
static int failing_reads;
static esp_err_t fail_init(void *ctx) { (void)ctx; return ESP_OK; }
static esp_err_t fail_set_integration(void *ctx, uint16_t ms) { (void)ctx; (void)ms; return ESP_OK; }
static esp_err_t fail_read(void *ctx, channel_reading_t *out)
{
    (void)ctx;
    if (++failing_reads >= 2) {
        return ESP_ERR_TIMEOUT;
    }
    *out = (channel_reading_t){.values = {500}, .count = 1};
    return ESP_OK;
}

static void test_run_publishes_once(void)
{
    strip_result_t r;
    TEST_ASSERT_EQUAL(ESP_OK, measurement_run(&deps, &cfg, &r));
    TEST_ASSERT_EQUAL(1, fake_sink_state()->publish_count);
    TEST_ASSERT_EQUAL(r.verdict, fake_sink_state()->last.verdict);
    TEST_ASSERT_EQUAL_FLOAT(r.score, fake_sink_state()->last.score);
    TEST_ASSERT_EQUAL_size_t(3, r.sample_count);
}

static void test_light_on_then_off(void)
{
    strip_result_t r;
    TEST_ASSERT_EQUAL(ESP_OK, measurement_run(&deps, &cfg, &r));
    fake_illuminator_state_t *l = fake_illuminator_state();
    TEST_ASSERT_EQUAL_size_t(2, l->call_count);
    TEST_ASSERT_TRUE(l->calls[0].on);
    TEST_ASSERT_EQUAL_UINT8(80, l->calls[0].intensity_pct);
    TEST_ASSERT_FALSE(l->calls[1].on);
}

static void test_settle_delay_requested(void)
{
    strip_result_t r;
    TEST_ASSERT_EQUAL(ESP_OK, measurement_run(&deps, &cfg, &r));
    TEST_ASSERT_EQUAL_UINT32(50, fake_timebase_total_ms());
}

static void test_invalid_sample_count(void)
{
    strip_result_t r;
    cfg.sample_count = 0;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, measurement_run(&deps, &cfg, &r));
    cfg.sample_count = MEASUREMENT_MAX_SAMPLES + 1;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, measurement_run(&deps, &cfg, &r));
    TEST_ASSERT_EQUAL(0, fake_sink_state()->publish_count);
    TEST_ASSERT_EQUAL_size_t(0, fake_illuminator_state()->call_count);
}

static void test_read_failure_turns_light_off_and_publishes_nothing(void)
{
    optical_sensor_t failing = {
        .init = fail_init,
        .set_integration_ms = fail_set_integration,
        .read = fail_read,
        .channel_count = 1,
    };
    failing_reads = 0;
    deps.sensor = &failing;

    strip_result_t r;
    TEST_ASSERT_EQUAL(ESP_ERR_TIMEOUT, measurement_run(&deps, &cfg, &r));
    fake_illuminator_state_t *l = fake_illuminator_state();
    TEST_ASSERT_TRUE(l->call_count >= 1);
    TEST_ASSERT_FALSE(l->calls[l->call_count - 1].on);
    TEST_ASSERT_EQUAL(0, fake_sink_state()->publish_count);
}

// Rows 0-1 are T samples, rows 2-3 are C samples.
static const channel_reading_t T_THEN_C[4] = {
    {.values = {100}, .count = 1},
    {.values = {300}, .count = 1},
    {.values = {60}, .count = 1},
    {.values = {80}, .count = 1},
};

static void test_lateral_flow_reads_t_then_c(void)
{
    deps.sensor = mock_sensor_create(T_THEN_C, 4);
    cfg.sample_count = 2;
    cfg.analysis = (strip_config_t){
        .mode = STRIP_MODE_LATERAL_FLOW, .t_threshold = 150.0f, .c_threshold = 50.0f};

    strip_result_t r;
    TEST_ASSERT_EQUAL(ESP_OK, measurement_run(&deps, &cfg, &r));
    TEST_ASSERT_EQUAL_FLOAT(200.0f, r.score);
    TEST_ASSERT_EQUAL_FLOAT(70.0f, r.control_score);
    TEST_ASSERT_EQUAL(STRIP_POSITIVE, r.verdict);
}

static void test_colorimetric_reads_one_set(void)
{
    deps.sensor = mock_sensor_create(T_THEN_C, 4);
    cfg.sample_count = 2;
    cfg.analysis = (strip_config_t){
        .mode = STRIP_MODE_COLORIMETRIC, .positive_threshold = 1000.0f, .min_signal = 10.0f};

    strip_result_t r;
    TEST_ASSERT_EQUAL(ESP_OK, measurement_run(&deps, &cfg, &r));
    TEST_ASSERT_EQUAL(STRIP_MODE_COLORIMETRIC, r.mode);
    TEST_ASSERT_EQUAL_FLOAT(200.0f, r.score);  // rows 0-1 only
    TEST_ASSERT_EQUAL_FLOAT(0.0f, r.control_score);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_run_publishes_once);
    RUN_TEST(test_light_on_then_off);
    RUN_TEST(test_settle_delay_requested);
    RUN_TEST(test_invalid_sample_count);
    RUN_TEST(test_read_failure_turns_light_off_and_publishes_nothing);
    RUN_TEST(test_lateral_flow_reads_t_then_c);
    RUN_TEST(test_colorimetric_reads_one_set);
    return UNITY_END();
}
