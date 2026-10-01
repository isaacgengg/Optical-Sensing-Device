#include "fake_illuminator.h"

static fake_illuminator_state_t s_state;
static illuminator_t s_light;

static esp_err_t fake_init(void *ctx)
{
    (void)ctx;
    s_state.init_count++;
    return ESP_OK;
}

static esp_err_t fake_set(void *ctx, bool on, uint8_t intensity_pct)
{
    (void)ctx;
    if (s_state.call_count < FAKE_ILLUMINATOR_MAX_CALLS) {
        s_state.calls[s_state.call_count] = (fake_light_call_t){on, intensity_pct};
    }
    s_state.call_count++;
    return ESP_OK;
}

illuminator_t *fake_illuminator_create(void)
{
    s_state = (fake_illuminator_state_t){0};
    s_light = (illuminator_t){.init = fake_init, .set = fake_set, .ctx = NULL};
    return &s_light;
}

fake_illuminator_state_t *fake_illuminator_state(void)
{
    return &s_state;
}
