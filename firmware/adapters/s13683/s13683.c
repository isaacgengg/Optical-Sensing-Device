// Register values from the S13683-02WT datasheet, KPIC1103E, "Register map"
// and "Program example". All I2C access goes through i2c_bus, never the
// ESP-IDF I2C API.

#include "s13683.h"

#define REG_CONTROL 0x00
#define REG_TIMING  0x01  // manual timing N, high byte then low byte (0x02)
#define REG_DATA    0x03  // R, G, B, correction: 4 x 16-bit, HIGH byte first

#define CTRL_ADC_RESET 0x80
#define CTRL_STANDBY   0x20  // read-only: set once a manual measurement is done
#define CTRL_HIGH_GAIN 0x08
#define CTRL_MANUAL    0x04  // manual setting mode: one measurement, then standby
#define CTRL_TINT_175US 0x00 // Tint = 00: manual base step 175 us

// TODO(team): low gain is the datasheet default. High gain is 10x more
// sensitive; switch to CTRL_HIGH_GAIN if strip readings come back too dim.
#define CTRL_CONFIG (CTRL_MANUAL | CTRL_TINT_175US)

// Integration time per channel = 175 us x N, N = 1..65535 (1 ms to ~11.4 s).
#define STEP_US 175

#define SATURATED 0xFFFF
#define POLL_MS 10
// ponytail: the sensor's integration clock isn't trimmed (the datasheet has a
// correction procedure for it), so allow 25% slack plus a fixed margin.
#define READY_MARGIN_MS 50

// Raw data index (R, G, B, correction) for each output channel, in
// wavelength order to match the TCS3448 driver's convention.
static const uint8_t k_out_to_raw[S13683_CHANNEL_COUNT] = {
    2,  // Blue
    1,  // Green
    0,  // Red
    3,  // Correction
};

typedef struct {
    i2c_bus_handle_t *bus;
    uint8_t address;
    i2c_bus_dev_t *dev;
    timebase_t *time;
    uint32_t integration_ms;
} s13683_ctx_t;

static s13683_ctx_t s_ctx;
static optical_sensor_t s_sensor;

static esp_err_t s13683_init(void *ctx)
{
    s13683_ctx_t *s = ctx;
    if (s->bus == NULL || s->time == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = ESP_OK;
    if (s->dev == NULL) {
        err = i2c_bus_add_device(s->bus, s->address, &s->dev);
        if (err != ESP_OK) {
            return err;
        }
    }
    // No ID register on this part. A successful read at least proves
    // something ACKs at the address.
    uint8_t ctrl;
    err = i2c_bus_read_regs(s->dev, REG_CONTROL, &ctrl, 1);
    if (err != ESP_OK) {
        return err;
    }
    // Leave the ADC in reset, out of standby, until the first read().
    return i2c_bus_write_reg(s->dev, REG_CONTROL, CTRL_ADC_RESET | CTRL_CONFIG);
}

static esp_err_t s13683_set_integration_ms(void *ctx, uint16_t ms)
{
    s13683_ctx_t *s = ctx;
    // Round to the nearest 175 us step.
    uint32_t n = ((uint32_t)ms * 1000 + STEP_US / 2) / STEP_US;
    if (n < 1 || n > 0xFFFF) {
        return ESP_ERR_INVALID_ARG;
    }
    const uint8_t timing[2] = {(uint8_t)(n >> 8), (uint8_t)(n & 0xFF)};
    esp_err_t err = i2c_bus_write_regs(s->dev, REG_TIMING, timing, sizeof(timing));
    if (err == ESP_OK) {
        s->integration_ms = ms;
    }
    return err;
}

static esp_err_t wait_done(s13683_ctx_t *s)
{
    // The four channels integrate back to back. Polling earlier could see
    // the standby bit still set from before the measurement started.
    uint32_t expected_ms = 4 * s->integration_ms;
    s->time->delay_ms(s->time->ctx, expected_ms);

    uint32_t budget_ms = expected_ms / 4 + READY_MARGIN_MS;
    for (uint32_t waited = 0; waited <= budget_ms; waited += POLL_MS) {
        uint8_t ctrl;
        esp_err_t err = i2c_bus_read_regs(s->dev, REG_CONTROL, &ctrl, 1);
        if (err != ESP_OK) {
            return err;
        }
        if (ctrl & CTRL_STANDBY) {
            return ESP_OK;
        }
        s->time->delay_ms(s->time->ctx, POLL_MS);
    }
    return ESP_ERR_TIMEOUT;
}

static esp_err_t s13683_read(void *ctx, channel_reading_t *out)
{
    s13683_ctx_t *s = ctx;
    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s->dev == NULL || s->integration_ms == 0) {
        return ESP_ERR_INVALID_STATE;  // init() and set_integration_ms() first
    }

    // Reset then release the ADC: starts one fresh measurement, so the data
    // can't predate the caller's setup (e.g. the LED turning on).
    esp_err_t err = i2c_bus_write_reg(s->dev, REG_CONTROL, CTRL_ADC_RESET | CTRL_CONFIG);
    if (err == ESP_OK) {
        err = i2c_bus_write_reg(s->dev, REG_CONTROL, CTRL_CONFIG);
    }
    if (err == ESP_OK) {
        err = wait_done(s);
    }
    uint8_t buf[2 * S13683_CHANNEL_COUNT];
    if (err == ESP_OK) {
        err = i2c_bus_read_regs(s->dev, REG_DATA, buf, sizeof(buf));
    }
    if (err != ESP_OK) {
        return err;
    }

    for (size_t c = 0; c < S13683_CHANNEL_COUNT; c++) {
        const uint8_t *raw = &buf[2 * k_out_to_raw[c]];
        uint16_t v = (uint16_t)((raw[0] << 8) | raw[1]);
        if (v == SATURATED) {
            return ESP_ERR_INVALID_RESPONSE;
        }
        out->values[c] = (float)v;
    }
    out->count = S13683_CHANNEL_COUNT;
    return ESP_OK;
}

optical_sensor_t *s13683_create(i2c_bus_handle_t *bus, uint8_t address, timebase_t *time)
{
    s_ctx = (s13683_ctx_t){.bus = bus, .address = address, .time = time};
    s_sensor = (optical_sensor_t){
        .init = s13683_init,
        .set_integration_ms = s13683_set_integration_ms,
        .read = s13683_read,
        .channel_count = S13683_CHANNEL_COUNT,
        .ctx = &s_ctx,
    };
    return &s_sensor;
}
