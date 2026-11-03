#include "freertos_timebase.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static timebase_t s_time;

static void rtos_delay_ms(void *ctx, uint32_t ms)
{
    (void)ctx;
    // Round up: pdMS_TO_TICKS truncates, so at 100 Hz a 5 ms delay became 0
    // ticks and polling loops spun through their whole budget instantly.
    vTaskDelay((ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS);
}

timebase_t *freertos_timebase_create(void)
{
    s_time = (timebase_t){.delay_ms = rtos_delay_ms, .ctx = NULL};
    return &s_time;
}
