#include "unity.h"
#include "tcs3448.h"
#include "as7343.h"
#include "fake_i2c_bus.h"
#include "fake_timebase.h"
void setUp(void) {}
void tearDown(void) {}
static void test_independent_devices_on_one_bus(void)
{
    i2c_bus_handle_t *bus = fake_i2c_bus_create();
    timebase_t *time = fake_timebase_create();
    optical_sensor_t *t = tcs3448_create(bus, 0x59, time);
    optical_sensor_t *c = as7343_create(bus, 0x39, time);
    TEST_ASSERT_NOT_EQUAL(t->ctx, c->ctx);
    fake_i2c_state_t *tr = fake_i2c_state_at(0x59), *cr = fake_i2c_state_at(0x39);
    tr->regs[0x5A] = cr->regs[0x5A] = 0x81;
    tr->regs[0x90] = cr->regs[0x90] = 0x40;
    tr->regs[0x95] = 42; cr->regs[0x95] = 99;  // FZ
    TEST_ASSERT_EQUAL(ESP_OK, t->init(t->ctx));
    TEST_ASSERT_EQUAL(ESP_OK, c->init(c->ctx));
    TEST_ASSERT_EQUAL(ESP_OK, t->set_integration_ms(t->ctx, 100));
    TEST_ASSERT_EQUAL(ESP_OK, c->set_integration_ms(c->ctx, 50));
    TEST_ASSERT_EQUAL(ESP_OK, t->set_gain(t->ctx, 256));
    TEST_ASSERT_EQUAL(ESP_OK, c->set_gain(c->ctx, 16));
    channel_reading_t a, b;
    TEST_ASSERT_EQUAL(ESP_OK, t->read(t->ctx, &a));
    TEST_ASSERT_EQUAL(ESP_OK, c->read(c->ctx, &b));
    TEST_ASSERT_EQUAL_FLOAT(42, a.values[2]);
    TEST_ASSERT_EQUAL_FLOAT(99, b.values[2]);
    TEST_ASSERT_EQUAL_HEX8(59, tr->regs[0x81]);
    TEST_ASSERT_EQUAL_HEX8(29, cr->regs[0x81]);
    TEST_ASSERT_EQUAL_HEX8(9, tr->regs[0xC6]);
    TEST_ASSERT_EQUAL_HEX8(5, cr->regs[0xC6]);
    cr->read_error = ESP_FAIL;
    TEST_ASSERT_EQUAL(ESP_FAIL, c->read(c->ctx, &b));
    TEST_ASSERT_EQUAL(ESP_OK, t->read(t->ctx, &a));
    TEST_ASSERT_EQUAL_FLOAT(42, a.values[2]);
}
int main(void) { UNITY_BEGIN(); RUN_TEST(test_independent_devices_on_one_bus); return UNITY_END(); }
