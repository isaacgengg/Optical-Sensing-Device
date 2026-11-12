// Composition root. Wiring only: create adapters, hand them to the use case,
// start the task. No logic lives here.

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "board.h"
#include "color_refs.h"
#include "lfa_refs.h"
#include "measurement.h"
#include "freertos_timebase.h"
#include "led_gpio.h"
#include "console_input.h"

#if CONFIG_DC_SENSOR_TCS3448
#include "i2c_bus.h"
#include "tcs3448.h"
#else
#include "mock_sensor.h"
#endif

#if CONFIG_DC_LFA_AS7343
#include "as7343.h"
#endif

#if CONFIG_DC_SINK_BLE
#include "ble_mobiledetect.h"
#else
#include "log_sink.h"
#endif

static const char *TAG = "main";

static measurement_deps_t s_deps;
static command_source_t *s_input;

#if CONFIG_DC_SENSOR_LEDS
// One illuminator for the use case: the GPIO LED plus each sensor board's
// onboard white LED. Every LED is set even if one fails, so "off" always
// reaches all of them; the first error is returned.
static illuminator_t *s_gpio_light;

static esp_err_t all_lights_init(void *ctx)
{
    (void)ctx;
    return s_gpio_light->init(s_gpio_light->ctx);
}

static esp_err_t all_lights_set(void *ctx, bool on, uint8_t intensity_pct)
{
    (void)ctx;
    on = on && intensity_pct > 0;
    uint16_t ma = (uint16_t)(CONFIG_DC_SENSOR_LED_MA * intensity_pct / 100);
    esp_err_t err = s_gpio_light->set(s_gpio_light->ctx, on, intensity_pct);
    esp_err_t e = tcs3448_set_led(on, ma);
    if (err == ESP_OK) err = e;
#if CONFIG_DC_LFA_AS7343
    if (s_deps.control_init_error == ESP_OK) {  // a missing AS7343 must not block colorimetric
        e = as7343_set_led(on, ma);
        if (err == ESP_OK) err = e;
    }
#endif
    return err;
}

static illuminator_t s_all_lights = {.init = all_lights_init, .set = all_lights_set};
#endif

// TODO(team): placeholder measurement parameters, one set per test mode.
// Tune once the optics exist.
static const measurement_config_t s_cfg[] = {
    [STRIP_MODE_LATERAL_FLOW] = {
        .integration_ms = LFA_TEST_INTEGRATION_MS,
        .gain = LFA_TEST_GAIN,
        .control_integration_ms = LFA_CONTROL_INTEGRATION_MS,
        .control_gain = LFA_CONTROL_GAIN,
        .led_intensity_pct = 100,
        .led_settle_ms = 50,
        .sample_count = 5,
        .analysis = {
            .mode = STRIP_MODE_LATERAL_FLOW,
            .test_ref = &LFA_TEST_REF,
            .control_ref = &LFA_CONTROL_REF,
            .lfa_format = LFA_COMPETITIVE,  // confirmed: C+T = negative, C only = positive
        },
    },
    [STRIP_MODE_COLORIMETRIC] = {
        .integration_ms = 100,
        .gain = 256,
        .led_intensity_pct = 100,
        .led_settle_ms = 50,
        .sample_count = 5,
        .analysis = {
            .mode = STRIP_MODE_COLORIMETRIC,
            .white = &COLOR_WHITE,          // see color_refs.h
            .refs = COLOR_REFS,
            .ref_count = sizeof(COLOR_REFS) / sizeof(COLOR_REFS[0]),
            .max_distance = COLOR_MAX_DISTANCE,
        },
    },
};

static void measurement_task(void *arg)
{
    (void)arg;
    for (;;) {
        strip_mode_t mode;
        strip_result_t result;
        esp_err_t err = s_input->wait_request(s_input->ctx, &mode);
        if (err == ESP_OK) {
            err = measurement_run(&s_deps, &s_cfg[mode], &result);
        }
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "measurement failed: %s", esp_err_to_name(err));
        }
    }
}

void app_main(void)
{
    s_deps.time = freertos_timebase_create();
    s_deps.light = led_gpio_create(BOARD_LED_GPIO);

#if CONFIG_DC_SENSOR_TCS3448
    i2c_bus_handle_t *bus = i2c_bus_init(BOARD_I2C_PORT, BOARD_I2C_SDA, BOARD_I2C_SCL, BOARD_I2C_HZ);
    if (bus == NULL) {
        ESP_LOGE(TAG, "I2C bus init failed");
        return;
    }
    i2c_bus_scan(bus);
    s_deps.color_sensor = tcs3448_create(bus, BOARD_TCS3448_I2C_ADDR, s_deps.time);
    s_deps.test_sensor = s_deps.color_sensor;
    ESP_LOGI(TAG, "source=hardware first/test=TCS3448@0x59 color=TCS3448@0x59");
#if CONFIG_DC_LFA_AS7343
    s_deps.control_sensor = as7343_create(bus, BOARD_AS7343_I2C_ADDR, s_deps.time);
    ESP_LOGI(TAG, "second/control=AS7343@0x39");
#endif
#if CONFIG_DC_SENSOR_LEDS
    s_gpio_light = s_deps.light;
    s_deps.light = &s_all_lights;
    ESP_LOGI(TAG, "light=GPIO%d + sensor onboard LEDs @ %d mA", BOARD_LED_GPIO, CONFIG_DC_SENSOR_LED_MA);
#endif
#else
    static mock_sensor_storage_t primary_mock, control_mock;
    static const channel_reading_t control_table[] = {
        {.values = {[4] = 500}, .count = 14},
    };
    s_deps.color_sensor = mock_sensor_create_in(&primary_mock, mock_default_table, mock_default_table_len);
    s_deps.test_sensor = s_deps.color_sensor;
    s_deps.control_sensor = mock_sensor_create_in(&control_mock, control_table, 1);
    ESP_LOGW(TAG, "source=MOCK first/test and second/control are simulated");
#endif

#if CONFIG_DC_SINK_BLE
    s_deps.sink = ble_mobiledetect_create();
#else
    s_deps.sink = log_sink_create();
#endif
    if (s_deps.sink == NULL) {
        ESP_LOGE(TAG, "result sink init failed");
        return;
    }

    esp_err_t err = measurement_init(&s_deps);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "measurement_init failed: %s", esp_err_to_name(err));
        return;
    }

    if (s_deps.control_init_error != ESP_OK) {
        ESP_LOGW(TAG, "LFA unavailable: second/control sensor init: %s; colorimetric remains available",
                 esp_err_to_name(s_deps.control_init_error));
    }
#if CONFIG_DC_LFA_AS7343
    if (LFA_TEST_REF.channel_mask == 0 || LFA_CONTROL_REF.channel_mask == 0) {
        ESP_LOGW(TAG, "LFA calibration required in lfa_refs.h; raw spectra available with 'l'");
    }
#endif

    s_input = console_input_create();
    err = s_input->init(s_input->ctx);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "input init failed: %s", esp_err_to_name(err));
        return;
    }

    // Samples are static; allow room for result structs and float logging.
    xTaskCreate(measurement_task, "measure", 8192, NULL, 5, NULL);
}
