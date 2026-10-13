#include <stdbool.h>
#include <string.h>
#include "fake_i2c_bus.h"

#define CFG0 0xBF
#define CFG0_REG_BANK 0x10

struct i2c_bus { int unused; };
struct i2c_bus_dev { int unused; };

static struct i2c_bus s_bus;
static struct i2c_bus_dev s_dev;
static fake_i2c_state_t s_state;

i2c_bus_handle_t *fake_i2c_bus_create(void)
{
    memset(&s_state, 0, sizeof(s_state));
    return &s_bus;
}

fake_i2c_state_t *fake_i2c_state(void)
{
    return &s_state;
}

esp_err_t i2c_bus_add_device(i2c_bus_handle_t *bus, uint8_t addr, i2c_bus_dev_t **dev_out)
{
    (void)addr;
    if (bus == NULL || dev_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *dev_out = &s_dev;
    return ESP_OK;
}

esp_err_t i2c_bus_read_regs(i2c_bus_dev_t *dev, uint8_t reg, uint8_t *buf, size_t len)
{
    if (dev == NULL || buf == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    bool bank1 = s_state.regs[CFG0] & CFG0_REG_BANK;
    for (size_t i = 0; i < len; i++) {
        uint8_t r = (uint8_t)(reg + i);
        bool banked = r >= 0x20 && r <= 0x7F;
        buf[i] = banked && !bank1 ? 0xEE : s_state.regs[r];
    }
    return ESP_OK;
}

esp_err_t i2c_bus_write_regs(i2c_bus_dev_t *dev, uint8_t reg, const uint8_t *buf, size_t len)
{
    if (dev == NULL || buf == NULL || len == 0 || len > I2C_BUS_MAX_WRITE) {
        return ESP_ERR_INVALID_ARG;
    }
    if (len > 1) {
        s_state.burst_count++;
    }
    for (size_t i = 0; i < len; i++) {
        uint8_t r = (uint8_t)(reg + i);
        s_state.regs[r] = buf[i];
        if (s_state.write_count < FAKE_I2C_MAX_WRITES) {
            s_state.writes[s_state.write_count].reg = r;
            s_state.writes[s_state.write_count].value = buf[i];
            s_state.write_count++;
        }
    }
    return ESP_OK;
}

esp_err_t i2c_bus_write_reg(i2c_bus_dev_t *dev, uint8_t reg, uint8_t value)
{
    return i2c_bus_write_regs(dev, reg, &value, 1);
}
