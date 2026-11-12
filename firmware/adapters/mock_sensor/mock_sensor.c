#include "mock_sensor.h"

typedef mock_sensor_storage_t mock_ctx_t;
static mock_sensor_storage_t s_storage;

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

static esp_err_t mock_set_gain(void *ctx, uint16_t gain)
{
    (void)ctx;
    return gain > 0 ? ESP_OK : ESP_ERR_INVALID_ARG;
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

optical_sensor_t *mock_sensor_create_in(mock_sensor_storage_t *storage,
                                       const channel_reading_t *table, size_t table_len)
{
    if (storage == NULL) return NULL;
    *storage = (mock_sensor_storage_t){.table = table, .table_len = table_len};
    storage->sensor = (optical_sensor_t){
        .init = mock_init, .set_integration_ms = mock_set_integration_ms,
        .set_gain = mock_set_gain, .read = mock_read,
        .channel_count = (table != NULL && table_len > 0) ? table[0].count : 0,
        .ctx = storage,
    };
    return &storage->sensor;
}

optical_sensor_t *mock_sensor_create(const channel_reading_t *table, size_t table_len)
{
    return mock_sensor_create_in(&s_storage, table, table_len);
}
