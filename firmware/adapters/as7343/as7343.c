// Register values from the AS7343 datasheet, DS001046 v6-00 (2023-Jun-07),
// section 10. All I2C access goes through i2c_bus, never the ESP-IDF I2C API.

#include "as7343.h"
#include "esp_log.h"

static const char *TAG = "as7343";

// Registers 0x58-0x66 need CFG0.REG_BANK = 1; everything else needs 0.
#define REG_ID       0x5A
#define REG_ENABLE   0x80
#define REG_ATIME    0x81
#define REG_STATUS2  0x90
#define REG_ASTATUS  0x94  // read 0x94..0xB8 in one burst: latches all 18 channels
#define REG_CFG0     0xBF
#define REG_CFG1     0xC6
#define REG_ASTEP_L  0xD4
#define REG_CFG20    0xD6
#define REG_LED      0xCD  // LED_ACT bit 7, LED_DRIVE bits 6:0

#define ID_VALUE           0x81
#define ENABLE_PON         0x01
#define ENABLE_ALS_EN      0x02
#define STATUS2_AVALID     0x40
#define ASTATUS_ASAT       0x80
#define CFG0_REG_BANK      0x10
#define CFG1_AGAIN_MASK    0x1F
#define CFG20_AUTO_SMUX_MASK 0x60
#define CFG20_AUTO_SMUX_18 0x60  // auto_smux = 3: 18 channels over 3 cycles
#define LED_ACT            0x80
#define LED_MIN_MA         4    // LED_DRIVE: I = 4 mA + 2 mA * code
#define LED_MAX_MA         258

// TODO(team): gain 9 = 256x (datasheet default; 0 = 0.5x ... 12 = 2048x).
// Tune once the LED and optics exist; lower it if reads report saturation.
#define DEFAULT_AGAIN 9

// t_int = (ATIME + 1) * (ASTEP + 1) * 2.78 us. ASTEP is fixed at 599, so one
// ATIME step is 1.668 ms and 600 counts of full scale (65535 cap reached at
// ATIME >= 109, ~182 ms). Range: 1.67 ms (ATIME 0) to 427 ms (ATIME 255).
// ponytail: fixed ASTEP; vary it too if sub-1.67 ms resolution ever matters.
#define ASTEP_VALUE   599
#define ATIME_STEP_US 1668

#define RAW_CHANNELS 18
#define POLL_MS      5
#define READY_MARGIN_MS 50  // SMUX switching and autozero on top of 3 integrations

// Raw data index (18-channel auto-SMUX order, CFG20 table) for each output
// channel, sorted by wavelength. Duplicate VIS/FD readings from cycles 2-3
// are dropped.
static const uint8_t k_out_to_raw[AS7343_CHANNEL_COUNT] = {
    12,  // F1
    6,   // F2
    0,   // FZ
    7,   // F3
    8,   // F4
    15,  // F5
    1,   // FY
    2,   // FXL
    9,   // F6
    13,  // F7
    14,  // F8
    3,   // NIR
    4,   // VIS
    5,   // FD
};

typedef struct {
    i2c_bus_handle_t *bus;
    uint8_t address;
    i2c_bus_dev_t *dev;
    timebase_t *time;
    uint32_t integration_ms;
    uint8_t gain_code;
} as7343_ctx_t;

static as7343_ctx_t s_ctx;
static optical_sensor_t s_sensor;

// Read-modify-write so reserved bits keep their values (datasheet section 10).
static esp_err_t update_bits(i2c_bus_dev_t *dev, uint8_t reg, uint8_t mask, uint8_t value)
{
    uint8_t cur;
    esp_err_t err = i2c_bus_read_regs(dev, reg, &cur, 1);
    if (err != ESP_OK) {
        return err;
    }
    return i2c_bus_write_reg(dev, reg, (uint8_t)((cur & ~mask) | (value & mask)));
}

static esp_err_t read_id(i2c_bus_dev_t *dev, uint8_t *id)
{
    esp_err_t err = update_bits(dev, REG_CFG0, CFG0_REG_BANK, CFG0_REG_BANK);
    if (err != ESP_OK) {
        return err;
    }
    err = i2c_bus_read_regs(dev, REG_ID, id, 1);
    // Always switch back, or every later register access hits the wrong bank.
    esp_err_t bank_err = update_bits(dev, REG_CFG0, CFG0_REG_BANK, 0);
    return err != ESP_OK ? err : bank_err;
}

static esp_err_t as7343_init(void *ctx)
{
    as7343_ctx_t *s = ctx;
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

    uint8_t id = 0;
    err = read_id(s->dev, &id);
    if (err != ESP_OK) {
        return err;
    }
    if (id != ID_VALUE) {
        return ESP_ERR_NOT_FOUND;
    }

    err = i2c_bus_write_reg(s->dev, REG_ENABLE, ENABLE_PON);
    if (err == ESP_OK) {
        err = update_bits(s->dev, REG_CFG20, CFG20_AUTO_SMUX_MASK, CFG20_AUTO_SMUX_18);
    }
    if (err == ESP_OK) {
        err = update_bits(s->dev, REG_CFG1, CFG1_AGAIN_MASK, DEFAULT_AGAIN);
    }
    if (err == ESP_OK) {
        err = i2c_bus_write_reg(s->dev, REG_LED, 0);  // onboard LED off until asked
    }
    s->integration_ms = 0;
    s->gain_code = DEFAULT_AGAIN;
    return err;
}

static esp_err_t as7343_set_integration_ms(void *ctx, uint16_t ms)
{
    as7343_ctx_t *s = ctx;
    if (s->dev == NULL) return ESP_ERR_INVALID_STATE;
    // Round to the nearest ATIME step.
    uint32_t steps = ((uint32_t)ms * 1000 + ATIME_STEP_US / 2) / ATIME_STEP_US;
    if (steps < 1 || steps > 256) {
        return ESP_ERR_INVALID_ARG;
    }
    // 16-bit field: low byte first, high byte immediately after, in one burst.
    const uint8_t astep[2] = {ASTEP_VALUE & 0xFF, ASTEP_VALUE >> 8};
    esp_err_t err = i2c_bus_write_regs(s->dev, REG_ASTEP_L, astep, sizeof(astep));
    if (err == ESP_OK) {
        err = i2c_bus_write_reg(s->dev, REG_ATIME, (uint8_t)(steps - 1));
    }
    if (err == ESP_OK) {
        s->integration_ms = ms;
    }
    return err;
}

static esp_err_t as7343_set_gain(void *ctx, uint16_t gain)
{
    as7343_ctx_t *s = ctx;
    if (s->dev == NULL) return ESP_ERR_INVALID_STATE;
    if (gain == 0 || gain > 2048 || (gain & (gain - 1)) != 0) return ESP_ERR_INVALID_ARG;
    uint8_t code = 1;  // 1x; register code 0 (0.5x) is not exposed by this port
    for (uint16_t g = gain; g > 1; g >>= 1) code++;
    esp_err_t err = update_bits(s->dev, REG_CFG1, CFG1_AGAIN_MASK, code);
    if (err == ESP_OK) s->gain_code = code;
    return err;
}

static esp_err_t wait_ready(as7343_ctx_t *s)
{
    uint32_t budget_ms = 3 * s->integration_ms + READY_MARGIN_MS;
    for (uint32_t waited = 0; waited < budget_ms; waited += POLL_MS) {
        uint8_t status2;
        esp_err_t err = i2c_bus_read_regs(s->dev, REG_STATUS2, &status2, 1);
        if (err != ESP_OK) {
            return err;
        }
        if (status2 & STATUS2_AVALID) {
            return ESP_OK;
        }
        s->time->delay_ms(s->time->ctx, POLL_MS);
    }
    return ESP_ERR_TIMEOUT;
}

static esp_err_t as7343_read(void *ctx, channel_reading_t *out)
{
    as7343_ctx_t *s = ctx;
    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s->dev == NULL || s->integration_ms == 0) {
        return ESP_ERR_INVALID_STATE;  // init() and set_integration_ms() first
    }

    // If the chip resets after init (brownout, loose wire) auto_smux falls back
    // to 6-channel mode and 8 of the 14 channels silently read 0. Check and
    // restore it every read. Warning once = chip reset; every read = the
    // write isn't taking.
    uint8_t cfg20 = 0;
    esp_err_t err = i2c_bus_read_regs(s->dev, REG_CFG20, &cfg20, 1);
    if (err == ESP_OK && (cfg20 & CFG20_AUTO_SMUX_MASK) != CFG20_AUTO_SMUX_18) {
        ESP_LOGW(TAG, "CFG20=0x%02x: auto_smux not 18-channel, rewriting", cfg20);
        err = i2c_bus_write_reg(s->dev, REG_CFG20,
                                (uint8_t)((cfg20 & ~CFG20_AUTO_SMUX_MASK) | CFG20_AUTO_SMUX_18));
    }

    if (err == ESP_OK) err = i2c_bus_write_reg(s->dev, REG_ENABLE, ENABLE_PON);
    if (err == ESP_OK) {
        s->time->delay_ms(s->time->ctx, 1);
        err = update_bits(s->dev, REG_CFG1, CFG1_AGAIN_MASK, s->gain_code);
    }
    if (err == ESP_OK) err = as7343_set_integration_ms(ctx, (uint16_t)s->integration_ms);

    // Start a fresh measurement so the data can't predate the caller's
    // setup (e.g. the LED turning on).
    if (err == ESP_OK) {
        err = i2c_bus_write_reg(s->dev, REG_ENABLE, ENABLE_PON | ENABLE_ALS_EN);
    }
    if (err == ESP_OK) {
        err = wait_ready(s);
    }
    uint8_t buf[1 + 2 * RAW_CHANNELS];  // ASTATUS, then CH0..CH17 low/high
    if (err == ESP_OK) {
        err = i2c_bus_read_regs(s->dev, REG_ASTATUS, buf, sizeof(buf));
    }
    // Stop measuring, always. A read error takes precedence.
    esp_err_t stop_err = i2c_bus_write_reg(s->dev, REG_ENABLE, ENABLE_PON);
    if (err != ESP_OK) {
        return err;
    }
    if (stop_err != ESP_OK) {
        return stop_err;
    }
    if (buf[0] & ASTATUS_ASAT) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    for (size_t c = 0; c < AS7343_CHANNEL_COUNT; c++) {
        const uint8_t *raw = &buf[1 + 2 * k_out_to_raw[c]];
        out->values[c] = (float)(raw[0] | (raw[1] << 8));
    }
    out->count = AS7343_CHANNEL_COUNT;
    return ESP_OK;
}

esp_err_t as7343_set_led(bool on, uint16_t ma)
{
    if (s_ctx.dev == NULL) return ESP_ERR_INVALID_STATE;  // init() first
    if (ma < LED_MIN_MA) ma = LED_MIN_MA;
    if (ma > LED_MAX_MA) ma = LED_MAX_MA;
    uint8_t code = (uint8_t)((ma - LED_MIN_MA) / 2);
    return i2c_bus_write_reg(s_ctx.dev, REG_LED, (uint8_t)((on ? LED_ACT : 0) | code));
}

optical_sensor_t *as7343_create(i2c_bus_handle_t *bus, uint8_t address, timebase_t *time)
{
    s_ctx = (as7343_ctx_t){.bus = bus, .address = address, .time = time};
    s_sensor = (optical_sensor_t){
        .init = as7343_init,
        .set_integration_ms = as7343_set_integration_ms,
        .read = as7343_read,
        .set_gain = as7343_set_gain,
        .channel_count = AS7343_CHANNEL_COUNT,
        .ctx = &s_ctx,
    };
    return &s_sensor;
}
