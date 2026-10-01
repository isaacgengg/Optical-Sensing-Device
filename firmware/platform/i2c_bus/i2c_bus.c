#include "i2c_bus.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

static const char *TAG = "i2c_bus";

#define I2C_BUS_TIMEOUT_MS  50
#define I2C_BUS_MAX_DEVICES 4

struct i2c_bus {
    i2c_master_bus_handle_t handle;
    uint32_t hz;
};

struct i2c_bus_dev {
    i2c_master_dev_handle_t handle;
    uint8_t addr;
};

static struct i2c_bus s_bus;
static struct i2c_bus_dev s_devs[I2C_BUS_MAX_DEVICES];
static size_t s_dev_count;

i2c_bus_handle_t *i2c_bus_init(int port, int sda, int scl, uint32_t hz)
{
    i2c_master_bus_config_t cfg = {
        .i2c_port = port,
        .sda_io_num = sda,
        .scl_io_num = scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,  // TODO(team): rely on board pull-ups once the PCB has them
    };
    esp_err_t err = i2c_new_master_bus(&cfg, &s_bus.handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "bus init failed: %s", esp_err_to_name(err));
        return NULL;
    }
    s_bus.hz = hz;
    return &s_bus;
}

esp_err_t i2c_bus_add_device(i2c_bus_handle_t *bus, uint8_t addr, i2c_bus_dev_t **dev_out)
{
    if (bus == NULL || dev_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_dev_count >= I2C_BUS_MAX_DEVICES) {
        return ESP_ERR_NO_MEM;
    }
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = bus->hz,
    };
    struct i2c_bus_dev *dev = &s_devs[s_dev_count];
    esp_err_t err = i2c_master_bus_add_device(bus->handle, &cfg, &dev->handle);
    if (err != ESP_OK) {
        return err;
    }
    dev->addr = addr;
    s_dev_count++;
    *dev_out = dev;
    return ESP_OK;
}

esp_err_t i2c_bus_read_regs(i2c_bus_dev_t *dev, uint8_t reg, uint8_t *buf, size_t len)
{
    if (dev == NULL || buf == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = ESP_FAIL;
    for (int attempt = 0; attempt < 2 && err != ESP_OK; attempt++) {
        err = i2c_master_transmit_receive(dev->handle, &reg, 1, buf, len, I2C_BUS_TIMEOUT_MS);
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "read addr=0x%02x reg=0x%02x failed: %s", dev->addr, reg, esp_err_to_name(err));
    }
    return err;
}

esp_err_t i2c_bus_write_reg(i2c_bus_dev_t *dev, uint8_t reg, uint8_t value)
{
    if (dev == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    const uint8_t frame[2] = {reg, value};
    esp_err_t err = ESP_FAIL;
    for (int attempt = 0; attempt < 2 && err != ESP_OK; attempt++) {
        err = i2c_master_transmit(dev->handle, frame, sizeof(frame), I2C_BUS_TIMEOUT_MS);
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "write addr=0x%02x reg=0x%02x failed: %s", dev->addr, reg, esp_err_to_name(err));
    }
    return err;
}

esp_err_t i2c_bus_scan(i2c_bus_handle_t *bus)
{
    if (bus == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    int found = 0;
    for (uint16_t addr = 0x08; addr < 0x78; addr++) {
        if (i2c_master_probe(bus->handle, addr, I2C_BUS_TIMEOUT_MS) == ESP_OK) {
            ESP_LOGI(TAG, "found device at 0x%02x", addr);
            found++;
        }
    }
    ESP_LOGI(TAG, "scan done, %d device(s)", found);
    return ESP_OK;
}
