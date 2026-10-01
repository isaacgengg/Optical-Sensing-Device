#include "fake_sink.h"

static fake_sink_state_t s_state;
static result_sink_t s_sink;

static esp_err_t fake_publish(void *ctx, const strip_result_t *result)
{
    (void)ctx;
    s_state.last = *result;
    s_state.publish_count++;
    return ESP_OK;
}

result_sink_t *fake_sink_create(void)
{
    s_state = (fake_sink_state_t){0};
    s_sink = (result_sink_t){.publish = fake_publish, .ctx = NULL};
    return &s_sink;
}

fake_sink_state_t *fake_sink_state(void)
{
    return &s_state;
}
