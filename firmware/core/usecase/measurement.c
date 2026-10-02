#include "measurement.h"
#include "strip_analysis.h"

// [0] = samples (T line for lateral flow), [1] = C line samples.
// ponytail: static so ~4.4 KB stays off the task stack; not reentrant, which is
// fine with one measurement task.
static channel_reading_t s_samples[2][MEASUREMENT_MAX_SAMPLES];

static esp_err_t read_samples(optical_sensor_t *sensor, channel_reading_t *out, size_t n)
{
    esp_err_t err = ESP_OK;
    for (size_t i = 0; i < n && err == ESP_OK; i++) {
        err = sensor->read(sensor->ctx, &out[i]);
    }
    return err;
}

static bool deps_valid(const measurement_deps_t *deps)
{
    return deps != NULL && deps->sensor != NULL && deps->light != NULL &&
           deps->time != NULL && deps->sink != NULL;
}

esp_err_t measurement_init(const measurement_deps_t *deps)
{
    if (!deps_valid(deps)) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = deps->sensor->init(deps->sensor->ctx);
    if (err != ESP_OK) {
        return err;
    }
    return deps->light->init(deps->light->ctx);
}

esp_err_t measurement_run(const measurement_deps_t *deps,
                          const measurement_config_t *cfg,
                          strip_result_t *out_result)
{
    // 1. Validate.
    if (!deps_valid(deps) || cfg == NULL || out_result == NULL ||
        cfg->sample_count == 0 || cfg->sample_count > MEASUREMENT_MAX_SAMPLES) {
        return ESP_ERR_INVALID_ARG;
    }

    optical_sensor_t *sensor = deps->sensor;
    illuminator_t *light = deps->light;

    // 2. Integration time.
    esp_err_t err = sensor->set_integration_ms(sensor->ctx, cfg->integration_ms);
    if (err != ESP_OK) {
        return err;
    }

    // 3. Light on.
    err = light->set(light->ctx, true, cfg->led_intensity_pct);
    if (err != ESP_OK) {
        light->set(light->ctx, false, 0);
        return err;
    }

    // 4. Settle.
    deps->time->delay_ms(deps->time->ctx, cfg->led_settle_ms);

    // 5. Sample. Lateral flow reads a T set, then a C set.
    // ponytail: stand-in until the optics decide how T and C are captured
    // separately (two sensors, two apertures, or a scan). Only this step changes.
    bool lateral_flow = cfg->analysis.mode == STRIP_MODE_LATERAL_FLOW;
    err = read_samples(sensor, s_samples[0], cfg->sample_count);
    if (err == ESP_OK && lateral_flow) {
        err = read_samples(sensor, s_samples[1], cfg->sample_count);
    }

    // 6. Light off, always. A read error takes precedence over an off error.
    esp_err_t off_err = light->set(light->ctx, false, 0);
    if (err != ESP_OK) {
        return err;
    }
    if (off_err != ESP_OK) {
        return off_err;
    }

    // 7. Analyze.
    strip_result_t result = lateral_flow
        ? strip_analyze_lateral_flow(s_samples[0], s_samples[1], cfg->sample_count, &cfg->analysis)
        : strip_analyze(s_samples[0], cfg->sample_count, &cfg->analysis);

    // 8. Publish and return.
    *out_result = result;
    return deps->sink->publish(deps->sink->ctx, &result);
}
