// Composition root. Wiring only: create adapters, hand them to the use case,
// start the task. No logic lives here.

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "board.h"
#include "measurement.h"
#include "freertos_timebase.h"
#include "led_gpio.h"
#include "console_input.h"

#if CONFIG_DC_SENSOR_AS7341
#include "i2c_bus.h"
#include "as7341.h"
#else
#include "mock_sensor.h"
#endif

#if CONFIG_DC_SINK_BLE
#include "ble_mobiledetect.h"
#else
#include "log_sink.h"
#endif

static const char *TAG = "main";

static measurement_deps_t s_deps;
static command_source_t *s_input;

// TODO(team): placeholder measurement parameters, one set per test mode.
// Tune once the optics exist.
static const measurement_config_t s_cfg[] = {
    [STRIP_MODE_LATERAL_FLOW] = {
        .integration_ms = 100,
        .led_intensity_pct = 100,
        .led_settle_ms = 50,
        .sample_count = 5,
        .analysis = {
            .mode = STRIP_MODE_LATERAL_FLOW,
            .positive_threshold = 1200.0f,  // TODO(team): placeholder
            .min_signal = 100.0f,           // TODO(team): placeholder
        },
    },
    [STRIP_MODE_COLORIMETRIC] = {
        .integration_ms = 100,
        .led_intensity_pct = 100,
        .led_settle_ms = 50,
        .sample_count = 5,
        .analysis = {
            .mode = STRIP_MODE_COLORIMETRIC,
            .positive_threshold = 1200.0f,  // TODO(team): placeholder
            .min_signal = 100.0f,           // TODO(team): placeholder
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

#if CONFIG_DC_SENSOR_AS7341
    i2c_bus_handle_t *bus = i2c_bus_init(BOARD_I2C_PORT, BOARD_I2C_SDA, BOARD_I2C_SCL, BOARD_I2C_HZ);
    if (bus == NULL) {
        ESP_LOGE(TAG, "I2C bus init failed");
        return;
    }
    i2c_bus_scan(bus);
    s_deps.sensor = as7341_create(bus, BOARD_SENSOR_I2C_ADDR);
#else
    s_deps.sensor = mock_sensor_create(mock_default_table, mock_default_table_len);
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

    s_input = console_input_create();
    err = s_input->init(s_input->ctx);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "input init failed: %s", esp_err_to_name(err));
        return;
    }

    // 8 KB: measurement_run keeps up to 32 samples (~2.2 KB) on the stack, and
    // printf of floats in the log sink needs ~1-2 KB more. 4 KB overflowed.
    xTaskCreate(measurement_task, "measure", 8192, NULL, 5, NULL);
}
