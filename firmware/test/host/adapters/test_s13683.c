#include "unity.h"
#include "s13683.h"
#include "fake_i2c_bus.h"
#include "fake_timebase.h"

static optical_sensor_t *sensor;
static fake_i2c_state_t *bus;

void setUp(void)
{
    sensor = s13683_create(fake_i2c_bus_create(), S13683_I2C_ADDR, fake_timebase_create());
    bus = fake_i2c_state();
    bus->regs[0x00] = 0xE4;  // power-on default
}

void tearDown(void) {}

static void init_and_configure(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, sensor->init(sensor->ctx));
    TEST_ASSERT_EQUAL(ESP_OK, sensor->set_integration_ms(sensor->ctx, 100));
}

static void test_init_configures_manual_mode_in_reset(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, sensor->init(sensor->ctx));
    TEST_ASSERT_EQUAL_HEX8(0x84, bus->regs[0x00]);  // ADC reset, manual, low gain, Tint 00
    TEST_ASSERT_EQUAL_size_t(4, sensor->channel_count);
}

static void test_integration_time_encoding(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, sensor->init(sensor->ctx));
    // Datasheet example: 546 ms = 3120 x 175 us = 0x0C30.
    TEST_ASSERT_EQUAL(ESP_OK, sensor->set_integration_ms(sensor->ctx, 546));
    TEST_ASSERT_EQUAL_HEX8(0x0C, bus->regs[0x01]);  // high byte first
    TEST_ASSERT_EQUAL_HEX8(0x30, bus->regs[0x02]);
    TEST_ASSERT_EQUAL_size_t(1, bus->burst_count);

    TEST_ASSERT_EQUAL(ESP_OK, sensor->set_integration_ms(sensor->ctx, 11468));  // N = 65531
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, sensor->set_integration_ms(sensor->ctx, 0));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, sensor->set_integration_ms(sensor->ctx, 11469));
}

static void test_read_maps_channels_in_wavelength_order(void)
{
    init_and_configure();
    bus->read_or[0x00] = 0x20;  // measurement done
    const uint16_t raw[4] = {0x0102, 0x0304, 0x0506, 0x0708};  // R G B correction
    for (int i = 0; i < 4; i++) {
        bus->regs[0x03 + 2 * i] = raw[i] >> 8;  // high byte first
        bus->regs[0x04 + 2 * i] = raw[i] & 0xFF;
    }

    channel_reading_t r;
    TEST_ASSERT_EQUAL(ESP_OK, sensor->read(sensor->ctx, &r));
    const float expected[4] = {0x0506, 0x0304, 0x0102, 0x0708};  // B G R correction
    TEST_ASSERT_EQUAL_size_t(4, r.count);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, r.values, 4);
}

static void test_read_resets_then_starts_and_waits_four_integrations(void)
{
    init_and_configure();
    bus->read_or[0x00] = 0x20;
    bus->write_count = 0;

    channel_reading_t r;
    TEST_ASSERT_EQUAL(ESP_OK, sensor->read(sensor->ctx, &r));
    TEST_ASSERT_EQUAL_size_t(2, bus->write_count);
    TEST_ASSERT_EQUAL_HEX8(0x84, bus->writes[0].value);  // ADC reset
    TEST_ASSERT_EQUAL_HEX8(0x04, bus->writes[1].value);  // release: start
    TEST_ASSERT_EQUAL_UINT32(400, fake_timebase_total_ms());  // R, G, B, correction
}

static void test_read_reports_saturation(void)
{
    init_and_configure();
    bus->read_or[0x00] = 0x20;
    bus->regs[0x05] = 0xFF;  // green = 65535
    bus->regs[0x06] = 0xFF;

    channel_reading_t r;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_RESPONSE, sensor->read(sensor->ctx, &r));
}

static void test_read_times_out_if_never_done(void)
{
    init_and_configure();  // 4 x 100 ms, then up to 400 / 4 + 50 ms of polling

    channel_reading_t r;
    TEST_ASSERT_EQUAL(ESP_ERR_TIMEOUT, sensor->read(sensor->ctx, &r));
    TEST_ASSERT_UINT32_WITHIN(10, 550, fake_timebase_total_ms());
}

static void test_read_before_configure_is_rejected(void)
{
    channel_reading_t r;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, sensor->read(sensor->ctx, &r));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_configures_manual_mode_in_reset);
    RUN_TEST(test_integration_time_encoding);
    RUN_TEST(test_read_maps_channels_in_wavelength_order);
    RUN_TEST(test_read_resets_then_starts_and_waits_four_integrations);
    RUN_TEST(test_read_reports_saturation);
    RUN_TEST(test_read_times_out_if_never_done);
    RUN_TEST(test_read_before_configure_is_rejected);
    return UNITY_END();
}
