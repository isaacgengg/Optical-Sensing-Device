#include <stdbool.h>
#include <string.h>
#include "fake_i2c_bus.h"

#define CFG0 0xBF
#define CFG0_REG_BANK 0x10

struct i2c_bus { int unused; };
struct i2c_bus_dev { uint8_t addr; };

static struct i2c_bus s_bus;
static struct i2c_bus_dev s_devs[128];
static fake_i2c_state_t s_states[128];

i2c_bus_handle_t *fake_i2c_bus_create(void)
{
    memset(s_states, 0, sizeof(s_states));
    return &s_bus;
}

fake_i2c_state_t *fake_i2c_state(void)
{
    return fake_i2c_state_at(0x59);
}

fake_i2c_state_t *fake_i2c_state_at(uint8_t addr)
{
    return addr < 128 ? &s_states[addr] : NULL;
}

esp_err_t i2c_bus_add_device(i2c_bus_handle_t *bus, uint8_t addr, i2c_bus_dev_t **dev_out)
{
    if (bus == NULL || dev_out == NULL || addr >= 128) {
        return ESP_ERR_INVALID_ARG;
    }
    s_devs[addr].addr = addr;
    *dev_out = &s_devs[addr];
    return ESP_OK;
}

esp_err_t i2c_bus_read_regs(i2c_bus_dev_t *dev, uint8_t reg, uint8_t *buf, size_t len)
{
    if (dev == NULL || buf == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    fake_i2c_state_t *state = &s_states[dev->addr];
    if (state->read_error != ESP_OK) return state->read_error;
    bool bank1 = state->regs[CFG0] & CFG0_REG_BANK;
    for (size_t i = 0; i < len; i++) {
        uint8_t r = (uint8_t)(reg + i);
        bool banked = r >= 0x20 && r <= 0x7F;
        buf[i] = banked && !bank1 ? 0xEE : state->regs[r];
    }
    return ESP_OK;
}

esp_err_t i2c_bus_write_regs(i2c_bus_dev_t *dev, uint8_t reg, const uint8_t *buf, size_t len)
{
    if (dev == NULL || buf == NULL || len == 0 || len > I2C_BUS_MAX_WRITE) {
        return ESP_ERR_INVALID_ARG;
    }
    fake_i2c_state_t *state = &s_states[dev->addr];
    if (state->write_error != ESP_OK) return state->write_error;
    if (len > 1) {
        state->burst_count++;
    }
    for (size_t i = 0; i < len; i++) {
        uint8_t r = (uint8_t)(reg + i);
        state->regs[r] = buf[i];
        if (state->write_count < FAKE_I2C_MAX_WRITES) {
            state->writes[state->write_count].reg = r;
            state->writes[state->write_count].value = buf[i];
            state->write_count++;
        }
    }
    return ESP_OK;
}

esp_err_t i2c_bus_write_reg(i2c_bus_dev_t *dev, uint8_t reg, uint8_t value)
{
    return i2c_bus_write_regs(dev, reg, &value, 1);
}
