// Port: light source. The core owns this interface; LED adapters
// (led_gpio, ...) implement it.
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

typedef struct {
    esp_err_t (*init)(void *ctx);
    esp_err_t (*set)(void *ctx, bool on, uint8_t intensity_pct);
    void *ctx;
} illuminator_t;
