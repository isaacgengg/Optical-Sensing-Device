#include "unity.h"
#include "tcs3448.h"
#include "fake_i2c_bus.h"
#include "fake_timebase.h"

static optical_sensor_t *sensor;
static fake_i2c_state_t *bus;

void setUp(void)
{
    sensor = tcs3448_create(fake_i2c_bus_create(), 0x59, fake_timebase_create());
    bus = fake_i2c_state();
    bus->regs[0x5A] = 0x81;  // ID, only visible with REG_BANK = 1
    bus->regs[0xD6] = 0x1F;  // CFG20 reserved bits set, to check they survive
    bus->regs[0xC6] = 0xE0;  // CFG1 reserved bits set
}

void tearDown(void) {}

static void init_and_configure(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, sensor->init(sensor->ctx));
    TEST_ASSERT_EQUAL(ESP_OK, sensor->set_integration_ms(sensor->ctx, 100));
}

static void test_init_reads_id_from_bank_1_and_configures(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, sensor->init(sensor->ctx));
    TEST_ASSERT_EQUAL_HEX8(0x00, bus->regs[0xBF] & 0x10);  // back on bank 0
    TEST_ASSERT_EQUAL_HEX8(0x01, bus->regs[0x80]);         // PON
    TEST_ASSERT_EQUAL_HEX8(0x7F, bus->regs[0xD6]);         // auto_smux = 3, reserved kept
    TEST_ASSERT_EQUAL_HEX8(0xE9, bus->regs[0xC6]);         // AGAIN = 9, reserved kept
    TEST_ASSERT_EQUAL_size_t(14, sensor->channel_count);
}

static void test_init_rejects_wrong_id(void)
{
    bus->regs[0x5A] = 0x24;
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, sensor->init(sensor->ctx));
    TEST_ASSERT_EQUAL_HEX8(0x00, bus->regs[0x80]);  // never powered on
}

static void test_integration_time_encoding(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, sensor->init(sensor->ctx));
    TEST_ASSERT_EQUAL(ESP_OK, sensor->set_integration_ms(sensor->ctx, 100));
    TEST_ASSERT_EQUAL_HEX8(59, bus->regs[0x81]);    // 60 steps x 1.668 ms = 100.1 ms
    TEST_ASSERT_EQUAL_HEX8(0x57, bus->regs[0xD4]);  // ASTEP 599, low byte
    TEST_ASSERT_EQUAL_HEX8(0x02, bus->regs[0xD5]);
    TEST_ASSERT_EQUAL_size_t(1, bus->burst_count);  // 16-bit field in one burst

    TEST_ASSERT_EQUAL(ESP_OK, sensor->set_integration_ms(sensor->ctx, 427));
    TEST_ASSERT_EQUAL_HEX8(255, bus->regs[0x81]);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, sensor->set_integration_ms(sensor->ctx, 0));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, sensor->set_integration_ms(sensor->ctx, 430));
}

static void test_read_maps_channels_in_wavelength_order(void)
{
    init_and_configure();
    bus->regs[0x90] = 0x40;  // AVALID
    bus->regs[0x94] = 0x00;  // ASTATUS: no saturation
    for (int raw = 0; raw < 18; raw++) {  // raw channel i reads 1000 + i
        uint16_t v = (uint16_t)(1000 + raw);
        bus->regs[0x95 + 2 * raw] = v & 0xFF;
        bus->regs[0x96 + 2 * raw] = v >> 8;
    }

    channel_reading_t r;
    TEST_ASSERT_EQUAL(ESP_OK, sensor->read(sensor->ctx, &r));
    // F1 F2 FZ F3 F4 F5 FY FXL F6 F7 F8 NIR VIS FD
    const float expected[14] = {1012, 1006, 1000, 1007, 1008, 1015, 1001,
                                1002, 1009, 1013, 1014, 1003, 1004, 1005};
    TEST_ASSERT_EQUAL_size_t(14, r.count);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, r.values, 14);
    TEST_ASSERT_EQUAL_HEX8(0x01, bus->regs[0x80]);  // measurement stopped
}

static void test_read_starts_a_fresh_measurement(void)
{
    init_and_configure();
    bus->regs[0x90] = 0x40;
    bus->write_count = 0;

    channel_reading_t r;
    TEST_ASSERT_EQUAL(ESP_OK, sensor->read(sensor->ctx, &r));
    TEST_ASSERT_EQUAL_size_t(2, bus->write_count);
    TEST_ASSERT_EQUAL_HEX8(0x80, bus->writes[0].reg);
    TEST_ASSERT_EQUAL_HEX8(0x03, bus->writes[0].value);  // PON | ALS_EN
    TEST_ASSERT_EQUAL_HEX8(0x01, bus->writes[1].value);  // PON
}

static void test_read_reports_saturation(void)
{
    init_and_configure();
    bus->regs[0x90] = 0x40;
    bus->regs[0x94] = 0x80;  // ASAT_STATUS

    channel_reading_t r;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_RESPONSE, sensor->read(sensor->ctx, &r));
    TEST_ASSERT_EQUAL_HEX8(0x01, bus->regs[0x80]);
}

static void test_read_times_out_without_data_ready(void)
{
    init_and_configure();  // 100 ms: budget is 3 x 100 + 50 ms

    channel_reading_t r;
    TEST_ASSERT_EQUAL(ESP_ERR_TIMEOUT, sensor->read(sensor->ctx, &r));
    TEST_ASSERT_UINT32_WITHIN(5, 350, fake_timebase_total_ms());
    TEST_ASSERT_EQUAL_HEX8(0x01, bus->regs[0x80]);
}

static void test_read_before_configure_is_rejected(void)
{
    channel_reading_t r;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, sensor->read(sensor->ctx, &r));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_reads_id_from_bank_1_and_configures);
    RUN_TEST(test_init_rejects_wrong_id);
    RUN_TEST(test_integration_time_encoding);
    RUN_TEST(test_read_maps_channels_in_wavelength_order);
    RUN_TEST(test_read_starts_a_fresh_measurement);
    RUN_TEST(test_read_reports_saturation);
    RUN_TEST(test_read_times_out_without_data_ready);
    RUN_TEST(test_read_before_configure_is_rejected);
    return UNITY_END();
}
