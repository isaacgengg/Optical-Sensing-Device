// timebase_t backed by vTaskDelay.
#pragma once
#include "timebase.h"

timebase_t *freertos_timebase_create(void);
