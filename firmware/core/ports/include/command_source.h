// Port: user input. The core owns this interface; input adapters
// (console_input, later a keypad or GPIO buttons) implement it.
#pragma once
#include "esp_err.h"

typedef enum {
    COMMAND_LATERAL_FLOW = 0,
    COMMAND_COLORIMETRIC,
    COMMAND_LFA_DARK,
} measurement_command_t;

typedef struct {
    esp_err_t (*init)(void *ctx);
    // Diagnostic captures are commands, not assay modes or verdicts.
    esp_err_t (*wait_request)(void *ctx, measurement_command_t *out_command);
    void *ctx;
} command_source_t;
