#include "log_sink.h"
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

static esp_err_t log_publish(void *ctx, const strip_result_t *result)
{
    (void)ctx;
    if (result == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_LOGI(TAG, "mode=%s verdict=%s score=%.2f control=%.2f samples=%u",
             mode_name(result->mode), verdict_name(result->verdict),
             (double)result->score, (double)result->control_score,
             (unsigned)result->sample_count);
    return ESP_OK;
}

result_sink_t *log_sink_create(void)
{
    s_sink = (result_sink_t){.publish = log_publish, .ctx = NULL};
    return &s_sink;
}
