// Result sink that writes results to the serial log. For bring-up without a phone.
#pragma once
#include "result_sink.h"

result_sink_t *log_sink_create(void);

// Serial-only calibration output: no verdict and no normal result frame.
esp_err_t log_sink_lfa_dark(const lfa_dark_capture_t *capture);
