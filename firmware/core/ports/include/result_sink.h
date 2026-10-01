// Port: result output. The core owns this interface; output adapters
// (log_sink, ble_mobiledetect, ...) implement it.
#pragma once
#include "esp_err.h"
#include "strip_types.h"

typedef struct {
    esp_err_t (*publish)(void *ctx, const strip_result_t *result);
    void *ctx;
} result_sink_t;
