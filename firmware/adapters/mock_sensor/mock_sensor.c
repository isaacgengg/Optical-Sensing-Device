#include "mock_sensor.h"

typedef struct {
    const channel_reading_t *table;
    size_t table_len;
    size_t next;
} mock_ctx_t;

static mock_ctx_t s_ctx;
static optical_sensor_t s_sensor;

static esp_err_t mock_init(void *ctx)
{
    mock_ctx_t *m = ctx;
    m->next = 0;
    return ESP_OK;
}

static esp_err_t mock_set_integration_ms(void *ctx, uint16_t ms)
{
    (void)ctx;
    (void)ms;
    return ESP_OK;
}

static esp_err_t mock_read(void *ctx, channel_reading_t *out)
{
    mock_ctx_t *m = ctx;
    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (m->table == NULL || m->table_len == 0) {
        return ESP_ERR_INVALID_STATE;
    }
    *out = m->table[m->next];
    m->next = (m->next + 1) % m->table_len;
    return ESP_OK;
}

optical_sensor_t *mock_sensor_create(const channel_reading_t *table, size_t table_len)
{
    s_ctx = (mock_ctx_t){.table = table, .table_len = table_len, .next = 0};
    s_sensor = (optical_sensor_t){
        .init = mock_init,
        .set_integration_ms = mock_set_integration_ms,
        .read = mock_read,
        .channel_count = (table != NULL && table_len > 0) ? table[0].count : 0,
        .ctx = &s_ctx,
    };
    return &s_sensor;
}
