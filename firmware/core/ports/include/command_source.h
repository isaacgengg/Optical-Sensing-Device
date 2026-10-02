// Port: user input. The core owns this interface; input adapters
// (console_input, later a keypad or GPIO buttons) implement it.
#pragma once
#include "esp_err.h"
#include "strip_types.h"

typedef struct {
    esp_err_t (*init)(void *ctx);
    // Blocks until the user asks for a measurement, then reports which mode.
    esp_err_t (*wait_request)(void *ctx, strip_mode_t *out_mode);
    void *ctx;
} command_source_t;
