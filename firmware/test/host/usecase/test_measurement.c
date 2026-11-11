#include "unity.h"
#include "measurement.h"
#include "mock_sensor.h"
#include "fake_illuminator.h"
#include "fake_timebase.h"
#include "fake_sink.h"

typedef struct {
    optical_sensor_t port;
    int id, inits, reads, configs;
    uint16_t ms, gain;
    float value;
    esp_err_t init_error, read_error, config_error;
} sensor_fake_t;
static sensor_fake_t t, c;
static int order[64], order_count;
static measurement_deps_t deps;
static measurement_config_t cfg;
static esp_err_t init(void *ctx) { sensor_fake_t *s = ctx; s->inits++; return s->init_error; }
static esp_err_t exposure(void *ctx, uint16_t ms)
{ sensor_fake_t *s = ctx; s->ms = ms; s->configs++; return s->config_error; }
static esp_err_t gain(void *ctx, uint16_t g)
{ sensor_fake_t *s = ctx; s->gain = g; return s->config_error; }
static esp_err_t read_sensor(void *ctx, channel_reading_t *out)
{
    sensor_fake_t *s = ctx;
    s->reads++; order[order_count++] = s->id;
    *out = (channel_reading_t){.values = {s->value}, .count = 1};
    return s->read_error;
}
static sensor_fake_t sensor(int id, float value)
{
    return (sensor_fake_t){.id = id, .value = value,
        .port = {.init = init, .read = read_sensor, .set_integration_ms = exposure,
                 .set_gain = gain, .channel_count = 1}};
}
static const lfa_zone_ref_t T_REF = {
    .dark = {.count = 1}, .blank = {.values = {1000}, .count = 1},
    .channel_mask = 1, .threshold = 0.5f,
};
static const lfa_zone_ref_t C_REF = {
    .dark = {.count = 1}, .blank = {.values = {2000}, .count = 1},
    .channel_mask = 1, .threshold = 0.3f,
};
static void color_mode(void)
{
    static const channel_reading_t white = {.values = {1000}, .count = 1};
    static const color_ref_t refs[] = {{.name = "red", .color = "red",
        .verdict = STRIP_POSITIVE, .reflectance = {0.2f}}};
    cfg.analysis = (strip_config_t){.mode = STRIP_MODE_COLORIMETRIC,
        .white = &white, .refs = refs, .ref_count = 1, .max_distance = 0.01f};
    cfg.integration_ms = 50; cfg.gain = 16;
}
void setUp(void)
{
    t = sensor(1, 200); c = sensor(2, 1000);
    t.port.ctx = &t; c.port.ctx = &c;
    order_count = 0;
    deps = (measurement_deps_t){.color_sensor = &t.port, .test_sensor = &t.port,
        .control_sensor = &c.port, .light = fake_illuminator_create(),
        .time = fake_timebase_create(), .sink = fake_sink_create()};
    cfg = (measurement_config_t){.integration_ms = 100, .gain = 256,
        .control_integration_ms = 75, .control_gain = 64,
        .led_intensity_pct = 100, .led_settle_ms = 50, .sample_count = 3,
        .analysis = {.mode = STRIP_MODE_LATERAL_FLOW, .test_ref = &T_REF,
            .control_ref = &C_REF, .lfa_format = LFA_COMPETITIVE}};
    TEST_ASSERT_EQUAL(ESP_OK, measurement_init(&deps));
}
void tearDown(void) {}
static void test_two_zones_alternate_and_publish_one_result(void)
{
    TEST_ASSERT_EQUAL(1, t.inits); TEST_ASSERT_EQUAL(1, c.inits);
    strip_result_t r;
    TEST_ASSERT_EQUAL(ESP_OK, measurement_run(&deps, &cfg, &r));
    const int expected[] = {1, 2, 1, 2, 1, 2};
    TEST_ASSERT_EQUAL_INT_ARRAY(expected, order, 6);
    TEST_ASSERT_EQUAL_FLOAT(200, r.raw.values[0]);
    TEST_ASSERT_EQUAL_FLOAT(1000, r.control_raw.values[0]);
    TEST_ASSERT_EQUAL_FLOAT(0.8f, r.score);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, r.control_score);
    TEST_ASSERT_EQUAL(STRIP_NEGATIVE, r.verdict);
    TEST_ASSERT_EQUAL(1, fake_sink_state()->publish_count);
    TEST_ASSERT_EQUAL(100, t.ms); TEST_ASSERT_EQUAL(75, c.ms);
    TEST_ASSERT_EQUAL(256, t.gain); TEST_ASSERT_EQUAL(64, c.gain);
    TEST_ASSERT_EQUAL(50, fake_timebase_total_ms());
    TEST_ASSERT_EQUAL(2, fake_illuminator_state()->call_count);
    TEST_ASSERT_FALSE(fake_illuminator_state()->calls[1].on);
}
static void test_color_never_configures_or_reads_control(void)
{
    color_mode();
    c.config_error = c.read_error = ESP_FAIL;
    strip_result_t r;
    TEST_ASSERT_EQUAL(ESP_OK, measurement_run(&deps, &cfg, &r));
    TEST_ASSERT_EQUAL(0, c.reads); TEST_ASSERT_EQUAL(0, c.configs);
    TEST_ASSERT_EQUAL(3, t.reads);
    TEST_ASSERT_EQUAL(STRIP_POSITIVE, r.verdict);
    TEST_ASSERT_EQUAL_STRING("red", r.color);
    TEST_ASSERT_EQUAL_size_t(0, r.control_raw.count);
}
static void test_failed_control_init_does_not_block_color(void)
{
    c.init_error = ESP_ERR_NOT_FOUND;
    TEST_ASSERT_EQUAL(ESP_OK, measurement_init(&deps));
    strip_result_t r;
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, measurement_run(&deps, &cfg, &r));
    TEST_ASSERT_EQUAL(0, fake_sink_state()->publish_count);
    color_mode();
    TEST_ASSERT_EQUAL(ESP_OK, measurement_run(&deps, &cfg, &r));
    TEST_ASSERT_EQUAL(0, c.reads);
}
static void test_missing_or_aliased_control_is_not_a_second_zone(void)
{
    deps.control_sensor = NULL;
    TEST_ASSERT_EQUAL(ESP_OK, measurement_init(&deps));
    strip_result_t r;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, measurement_run(&deps, &cfg, &r));
    deps.control_sensor = deps.test_sensor;
    TEST_ASSERT_EQUAL(ESP_OK, measurement_init(&deps));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, measurement_run(&deps, &cfg, &r));
    color_mode();
    TEST_ASSERT_EQUAL(ESP_OK, measurement_run(&deps, &cfg, &r));
}
static void test_read_failure_leaves_light_off_and_no_result(void)
{
    c.read_error = ESP_ERR_TIMEOUT;
    strip_result_t r = {.verdict = STRIP_POSITIVE};
    TEST_ASSERT_EQUAL(ESP_ERR_TIMEOUT, measurement_run(&deps, &cfg, &r));
    TEST_ASSERT_EQUAL(STRIP_INVALID, r.verdict);
    TEST_ASSERT_EQUAL(0, fake_sink_state()->publish_count);
    TEST_ASSERT_FALSE(fake_illuminator_state()->calls[1].on);
    c.read_error = ESP_OK; t.read_error = ESP_FAIL;
    TEST_ASSERT_EQUAL(ESP_FAIL, measurement_run(&deps, &cfg, &r));
    TEST_ASSERT_FALSE(fake_illuminator_state()->calls[3].on);
    TEST_ASSERT_EQUAL(0, fake_sink_state()->publish_count);
}
static void test_mode_switch_restores_each_sensor_settings(void)
{
    strip_result_t r;
    measurement_config_t lfa = cfg;
    color_mode();
    TEST_ASSERT_EQUAL(ESP_OK, measurement_run(&deps, &cfg, &r));
    TEST_ASSERT_EQUAL(50, t.ms); TEST_ASSERT_EQUAL(16, t.gain);
    TEST_ASSERT_EQUAL(ESP_OK, measurement_run(&deps, &lfa, &r));
    TEST_ASSERT_EQUAL(100, t.ms); TEST_ASSERT_EQUAL(256, t.gain);
    TEST_ASSERT_EQUAL(75, c.ms); TEST_ASSERT_EQUAL(64, c.gain);
    TEST_ASSERT_EQUAL(ESP_OK, measurement_run(&deps, &cfg, &r));
    TEST_ASSERT_EQUAL(50, t.ms); TEST_ASSERT_EQUAL(3, c.reads);
}
static void test_config_failure_and_bad_inputs_do_not_publish(void)
{
    strip_result_t r;
    c.config_error = ESP_FAIL;
    TEST_ASSERT_EQUAL(ESP_FAIL, measurement_run(&deps, &cfg, &r));
    TEST_ASSERT_EQUAL(0, order_count);
    TEST_ASSERT_EQUAL(0, fake_illuminator_state()->call_count);
    cfg.sample_count = 0;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, measurement_run(&deps, &cfg, &r));
    cfg.sample_count = MEASUREMENT_MAX_SAMPLES + 1;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, measurement_run(&deps, &cfg, &r));
    cfg.sample_count = 1; cfg.analysis.mode = (strip_mode_t)99;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, measurement_run(&deps, &cfg, &r));
    TEST_ASSERT_EQUAL(0, fake_sink_state()->publish_count);
}
static void test_uncalibrated_lfa_logs_both_zones_as_invalid(void)
{
    cfg.analysis.test_ref = NULL;
    strip_result_t r;
    TEST_ASSERT_EQUAL(ESP_OK, measurement_run(&deps, &cfg, &r));
    TEST_ASSERT_EQUAL(STRIP_INVALID, r.verdict);
    TEST_ASSERT_FALSE(r.lfa_calibration_valid);
    TEST_ASSERT_EQUAL_FLOAT(200, r.raw.values[0]);
    TEST_ASSERT_EQUAL_FLOAT(1000, r.control_raw.values[0]);
    TEST_ASSERT_EQUAL_FLOAT(0, r.score); // finite, BLE v1 compatible
}
static void test_mock_instances_have_independent_streams(void)
{
    mock_sensor_storage_t a, b;
    const channel_reading_t ar[] = {{.values = {1}, .count = 1}, {.values = {2}, .count = 1}};
    const channel_reading_t br[] = {{.values = {9}, .count = 1}};
    optical_sensor_t *as = mock_sensor_create_in(&a, ar, 2), *bs = mock_sensor_create_in(&b, br, 1);
    channel_reading_t raw;
    TEST_ASSERT_EQUAL(ESP_OK, as->init(as->ctx)); TEST_ASSERT_EQUAL(ESP_OK, bs->init(bs->ctx));
    as->read(as->ctx, &raw); TEST_ASSERT_EQUAL_FLOAT(1, raw.values[0]);
    bs->read(bs->ctx, &raw); TEST_ASSERT_EQUAL_FLOAT(9, raw.values[0]);
    as->read(as->ctx, &raw); TEST_ASSERT_EQUAL_FLOAT(2, raw.values[0]);
}
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_two_zones_alternate_and_publish_one_result);
    RUN_TEST(test_color_never_configures_or_reads_control);
    RUN_TEST(test_failed_control_init_does_not_block_color);
    RUN_TEST(test_missing_or_aliased_control_is_not_a_second_zone);
    RUN_TEST(test_read_failure_leaves_light_off_and_no_result);
    RUN_TEST(test_mode_switch_restores_each_sensor_settings);
    RUN_TEST(test_config_failure_and_bad_inputs_do_not_publish);
    RUN_TEST(test_uncalibrated_lfa_logs_both_zones_as_invalid);
    RUN_TEST(test_mock_instances_have_independent_streams);
    return UNITY_END();
}
