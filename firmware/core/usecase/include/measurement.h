// Use case: take a measurement and report it. Talks to the outside world
// only through the ports in measurement_deps_t.
#pragma once
#include "optical_sensor.h"
#include "illuminator.h"
#include "timebase.h"
#include "result_sink.h"
#include "strip_types.h"

typedef struct {
    optical_sensor_t *sensor;
    illuminator_t *light;
    timebase_t *time;
    result_sink_t *sink;
} measurement_deps_t;

typedef struct {
    uint16_t integration_ms;
    uint8_t led_intensity_pct;
    uint32_t led_settle_ms;
    size_t sample_count;        // must be <= MEASUREMENT_MAX_SAMPLES
    strip_config_t analysis;
} measurement_config_t;

#define MEASUREMENT_MAX_SAMPLES 32

esp_err_t measurement_init(const measurement_deps_t *deps);
esp_err_t measurement_run(const measurement_deps_t *deps,
                          const measurement_config_t *cfg,
                          strip_result_t *out_result);
