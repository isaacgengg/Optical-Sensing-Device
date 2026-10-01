// Port: optical sensor. The core owns this interface; sensor adapters
// (mock_sensor, as7341, ...) implement it.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "strip_types.h"

typedef struct {
    esp_err_t (*init)(void *ctx);
    esp_err_t (*set_integration_ms)(void *ctx, uint16_t ms);
    // Blocks until a fresh reading is available, then fills `out`.
    esp_err_t (*read)(void *ctx, channel_reading_t *out);
    size_t channel_count;
    void *ctx;
} optical_sensor_t;
