#include <stddef.h>
#include "fake_timebase.h"

static uint32_t s_total_ms;
static timebase_t s_time;

static void fake_delay_ms(void *ctx, uint32_t ms)
{
    (void)ctx;
    s_total_ms += ms;
}

timebase_t *fake_timebase_create(void)
{
    s_total_ms = 0;
    s_time = (timebase_t){.delay_ms = fake_delay_ms, .ctx = NULL};
    return &s_time;
}

uint32_t fake_timebase_total_ms(void)
{
    return s_total_ms;
}
