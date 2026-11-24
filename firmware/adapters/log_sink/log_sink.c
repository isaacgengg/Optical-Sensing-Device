#include "log_sink.h"
#include <stdio.h>
#include "esp_log.h"

static const char *TAG = "log_sink";

static result_sink_t s_sink;

static const char *verdict_name(strip_verdict_t v)
{
    switch (v) {
    case STRIP_NEGATIVE: return "NEGATIVE";
    case STRIP_POSITIVE: return "POSITIVE";
    case STRIP_INVALID:  return "INVALID";
    }
    return "?";
}

static const char *mode_name(strip_mode_t m)
{
    switch (m) {
    case STRIP_MODE_LATERAL_FLOW: return "LATERAL_FLOW";
    case STRIP_MODE_COLORIMETRIC: return "COLORIMETRIC";
    }
    return "?";
}

static const char *color_status_name(color_match_status_t status)
{
    switch (status) {
    case COLOR_MATCH_ACCEPTED: return "matched";
    case COLOR_MATCH_TOO_FAR: return "too_far";
    case COLOR_MATCH_AMBIGUOUS: return "ambiguous";
    case COLOR_MATCH_NOT_EVALUATED: return "not_evaluated";
    }
    return "not_evaluated";
}

// Comma-separated channel values, e.g. "179, 422, 1649, ...", paste-ready for
// firmware/main/color_refs.h.
static void log_channels(const char *label, const channel_reading_t *r, const char *fmt)
{
    if (r->count == 0) {
        return;
    }
    char buf[16 * DC_MAX_CHANNELS];
    size_t len = 0;
    for (size_t c = 0; c < r->count && len < sizeof(buf); c++) {
        len += snprintf(buf + len, sizeof(buf) - len, fmt, (double)r->values[c]);
    }
    ESP_LOGI(TAG, "%s=%s", label, buf);
}

static esp_err_t log_publish(void *ctx, const strip_result_t *result)
{
    (void)ctx;
    if (result == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_LOGI(TAG, "mode=%s verdict=%s match=%s color=%s score=%.3f control=%.2f samples=%u",
             mode_name(result->mode), verdict_name(result->verdict),
             result->match ? result->match : "none",
             result->color ? result->color : "none",
             (double)result->score, (double)result->control_score,
             (unsigned)result->sample_count);
    if (result->mode == STRIP_MODE_LATERAL_FLOW) {
        ESP_LOGI(TAG, "zones: first=test second=control; metric=contrast calibration=%s",
                 result->lfa_calibration_valid ? "valid" : "missing_or_invalid");
        log_channels("test_raw", &result->raw, "%.0f, ");
        log_channels("control_raw", &result->control_raw, "%.0f, ");
    } else {
        const color_match_info_t *info = &result->color_match;
        if (info->status != COLOR_MATCH_NOT_EVALUATED) {
            ESP_LOGI(TAG, "color_match: status=%s nearest=%s runner_up=%s runner_up_score=%.6f margin=%.6f max_distance=%.3f min_margin=%.3f",
                     color_status_name(info->status), info->nearest ? info->nearest : "none",
                     info->runner_up ? info->runner_up : "none", (double)info->runner_up_score,
                     (double)info->margin, (double)info->max_distance, (double)info->min_margin);
        }
        log_channels("raw", &result->raw, "%.0f, ");
    }
    log_channels("refl", &result->reflectance, "%.3f, ");
    return ESP_OK;
}

esp_err_t log_sink_lfa_dark(const lfa_dark_capture_t *capture)
{
    if (!capture || capture->sample_count == 0 ||
        capture->test_raw.count == 0 || capture->test_raw.count > DC_MAX_CHANNELS ||
        capture->control_raw.count == 0 || capture->control_raw.count > DC_MAX_CHANNELS)
        return ESP_ERR_INVALID_ARG;
    ESP_LOGI(TAG, "capture=LFA_DARK illumination=off samples=%u test_gain=%u test_ms=%u control_gain=%u control_ms=%u",
             (unsigned)capture->sample_count, (unsigned)capture->test_gain,
             (unsigned)capture->test_integration_ms, (unsigned)capture->control_gain,
             (unsigned)capture->control_integration_ms);
    // Preserve sub-count averages: dark values can be smaller than one count.
    log_channels("dark_test_raw", &capture->test_raw, "%.3f, ");
    log_channels("dark_control_raw", &capture->control_raw, "%.3f, ");
    return ESP_OK;
}

result_sink_t *log_sink_create(void)
{
    s_sink = (result_sink_t){.publish = log_publish, .ctx = NULL};
    return &s_sink;
}
