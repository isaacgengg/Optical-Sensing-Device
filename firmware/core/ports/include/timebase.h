// Port: time. The core owns this interface so the use case can wait without
// calling FreeRTOS directly; freertos_timebase implements it on the device.
#pragma once
#include <stdint.h>

typedef struct {
    void (*delay_ms)(void *ctx, uint32_t ms);
    void *ctx;
} timebase_t;
