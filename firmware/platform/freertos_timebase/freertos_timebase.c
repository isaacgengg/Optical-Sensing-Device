#include "freertos_timebase.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static timebase_t s_time;

static void rtos_delay_ms(void *ctx, uint32_t ms)
{
    (void)ctx;
    vTaskDelay(pdMS_TO_TICKS(ms));
}

timebase_t *freertos_timebase_create(void)
{
    s_time = (timebase_t){.delay_ms = rtos_delay_ms, .ctx = NULL};
    return &s_time;
}
