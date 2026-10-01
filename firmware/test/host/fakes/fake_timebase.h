#pragma once
#include "timebase.h"

// Resets the total. delay_ms records but never sleeps.
timebase_t *fake_timebase_create(void);
uint32_t fake_timebase_total_ms(void);
