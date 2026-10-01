// PLACEHOLDER: sensor part not confirmed. This is a skeleton.
// Every function returns ESP_ERR_NOT_SUPPORTED until it is filled in.
// All I2C access must go through i2c_bus, never the ESP-IDF I2C API directly.

#include "as7341.h"

// TODO(team): fill in from datasheet (register addresses, ID value, gain and
// integration encodings, channel count). Do not guess these.

typedef struct {
    i2c_bus_handle_t *bus;
    uint8_t address;
    i2c_bus_dev_t *dev;
} as7341_ctx_t;

static as7341_ctx_t s_ctx;
static optical_sensor_t s_sensor;

static esp_err_t as7341_init(void *ctx)
{
    (void)ctx;
    // TODO(team): add the device with i2c_bus_add_device(bus, address, &dev),
    // read and check the ID register, power on, configure default gain.
    return ESP_ERR_NOT_SUPPORTED;
}

static esp_err_t as7341_set_integration_ms(void *ctx, uint16_t ms)
{
    (void)ctx;
    (void)ms;
    // TODO(team): convert `ms` to the sensor's integration-time register
    // encoding and write it via i2c_bus_write_reg.
    return ESP_ERR_NOT_SUPPORTED;
}

static esp_err_t as7341_read(void *ctx, channel_reading_t *out)
{
    (void)ctx;
    (void)out;
    // TODO(team): start a measurement, wait for data ready (poll status or
    // use the INT pin), read the channel registers with i2c_bus_read_regs,
    // convert raw counts to floats, set out->count.
    return ESP_ERR_NOT_SUPPORTED;
}

optical_sensor_t *as7341_create(i2c_bus_handle_t *bus, uint8_t address)
{
    s_ctx = (as7341_ctx_t){.bus = bus, .address = address, .dev = NULL};
    s_sensor = (optical_sensor_t){
        .init = as7341_init,
        .set_integration_ms = as7341_set_integration_ms,
        .read = as7341_read,
        .channel_count = 0,  // TODO(team): fill in from datasheet
        .ctx = &s_ctx,
    };
    return &s_sensor;
}
