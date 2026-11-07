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
    log_channels("raw", &result->raw, "%.0f, ");
    log_channels("refl", &result->reflectance, "%.3f, ");
    return ESP_OK;
}

result_sink_t *log_sink_create(void)
{
    s_sink = (result_sink_t){.publish = log_publish, .ctx = NULL};
    return &s_sink;
}
