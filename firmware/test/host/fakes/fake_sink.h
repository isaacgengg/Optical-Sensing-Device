#pragma once
#include "result_sink.h"

typedef struct {
    strip_result_t last;
    int publish_count;
} fake_sink_state_t;

// Resets recorded state.
result_sink_t *fake_sink_create(void);
fake_sink_state_t *fake_sink_state(void);
