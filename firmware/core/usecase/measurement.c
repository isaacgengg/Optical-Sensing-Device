#include "measurement.h"
#include "strip_analysis.h"

// Single measurement task. Separate buffers, never intermix the two zones.
static channel_reading_t s_samples[2][MEASUREMENT_MAX_SAMPLES];

static bool sensor_valid(const optical_sensor_t *s)
{
    return s && s->init && s->set_integration_ms && s->set_gain && s->read;
}

static bool deps_valid(const measurement_deps_t *d)
{
    return d && sensor_valid(d->color_sensor) && sensor_valid(d->test_sensor) &&
           d->light && d->light->init && d->light->set && d->time &&
           d->time->delay_ms && d->sink && d->sink->publish;
}

esp_err_t measurement_init(measurement_deps_t *d)
{
    if (!deps_valid(d)) return ESP_ERR_INVALID_ARG;
    esp_err_t err = d->color_sensor->init(d->color_sensor->ctx);
    if (err == ESP_OK && d->test_sensor != d->color_sensor)
        err = d->test_sensor->init(d->test_sensor->ctx);
    if (err != ESP_OK) return err;
    // A missing secondary must not prevent TCS-only color measurements.
    d->control_init_error = ESP_ERR_NOT_FOUND;
    if (sensor_valid(d->control_sensor) && d->control_sensor != d->test_sensor &&
        d->control_sensor != d->color_sensor)
        d->control_init_error = d->control_sensor->init(d->control_sensor->ctx);
    return d->light->init(d->light->ctx);
}

static esp_err_t configure(optical_sensor_t *sensor, uint16_t ms, uint16_t gain)
{
    esp_err_t err = sensor->set_gain(sensor->ctx, gain);
    if (err == ESP_OK) err = sensor->set_integration_ms(sensor->ctx, ms);
    return err;
}

esp_err_t measurement_run(const measurement_deps_t *d,
                          const measurement_config_t *cfg, strip_result_t *out)
{
    if (!deps_valid(d) || !cfg || !out || cfg->sample_count == 0 ||
        cfg->sample_count > MEASUREMENT_MAX_SAMPLES ||
        (cfg->analysis.mode != STRIP_MODE_COLORIMETRIC &&
         cfg->analysis.mode != STRIP_MODE_LATERAL_FLOW)) return ESP_ERR_INVALID_ARG;
    *out = (strip_result_t){.mode = cfg->analysis.mode, .verdict = STRIP_INVALID};
    bool lfa = cfg->analysis.mode == STRIP_MODE_LATERAL_FLOW;
    if (lfa && (!sensor_valid(d->control_sensor) || d->control_sensor == d->test_sensor))
        return ESP_ERR_INVALID_STATE;
    if (lfa && d->control_init_error != ESP_OK) return d->control_init_error;
    optical_sensor_t *primary = lfa ? d->test_sensor : d->color_sensor;
    esp_err_t err = configure(primary, cfg->integration_ms, cfg->gain);
    if (err == ESP_OK && lfa)
        err = configure(d->control_sensor, cfg->control_integration_ms, cfg->control_gain);
    if (err != ESP_OK) return err;

    err = d->light->set(d->light->ctx, true, cfg->led_intensity_pct);
    if (err == ESP_OK) {
        d->time->delay_ms(d->time->ctx, cfg->led_settle_ms);
        for (size_t i = 0; i < cfg->sample_count && err == ESP_OK; i++) {
            // First physical line = TCS/test, second = AS7343/control.
            err = primary->read(primary->ctx, &s_samples[0][i]);
            if (err == ESP_OK && lfa)
                err = d->control_sensor->read(d->control_sensor->ctx, &s_samples[1][i]);
        }
    }
    esp_err_t off_err = d->light->set(d->light->ctx, false, 0);
    if (err != ESP_OK) return err;
    if (off_err != ESP_OK) return off_err;

    *out = lfa ? strip_analyze_lateral_flow(s_samples[0], s_samples[1], cfg->sample_count, &cfg->analysis)
               : strip_analyze(s_samples[0], cfg->sample_count, &cfg->analysis);
    return d->sink->publish(d->sink->ctx, out);
}
