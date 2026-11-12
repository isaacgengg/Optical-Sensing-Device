// Use case: take a measurement and report it. Talks to the outside world
// only through the ports in measurement_deps_t.
#pragma once
#include "optical_sensor.h"
#include "illuminator.h"
#include "timebase.h"
#include "result_sink.h"
#include "strip_types.h"

typedef struct {
    optical_sensor_t *color_sensor;
    optical_sensor_t *test_sensor;
    optical_sensor_t *control_sensor;  // may be NULL for TCS-only hardware
    esp_err_t control_init_error;     // recorded by measurement_init
    illuminator_t *light;
    timebase_t *time;
    result_sink_t *sink;
} measurement_deps_t;

typedef struct {
    uint16_t integration_ms;  // primary TCS exposure, per mode
    uint16_t gain;
    uint16_t control_integration_ms;
    uint16_t control_gain;
    uint8_t led_intensity_pct;
    uint32_t led_settle_ms;
    size_t sample_count;        // must be <= MEASUREMENT_MAX_SAMPLES
    strip_config_t analysis;
} measurement_config_t;

#define MEASUREMENT_MAX_SAMPLES 32

esp_err_t measurement_init(measurement_deps_t *deps);
esp_err_t measurement_run(const measurement_deps_t *deps,
                          const measurement_config_t *cfg,
                          strip_result_t *out_result);
