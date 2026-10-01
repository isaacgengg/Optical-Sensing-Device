#pragma once
#include <stdbool.h>
#include <stddef.h>
#include "illuminator.h"

#define FAKE_ILLUMINATOR_MAX_CALLS 16

typedef struct {
    bool on;
    uint8_t intensity_pct;
} fake_light_call_t;

typedef struct {
    fake_light_call_t calls[FAKE_ILLUMINATOR_MAX_CALLS];
    size_t call_count;
    int init_count;
} fake_illuminator_state_t;

// Resets recorded state.
illuminator_t *fake_illuminator_create(void);
fake_illuminator_state_t *fake_illuminator_state(void);
