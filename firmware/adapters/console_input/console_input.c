#include "console_input.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "driver/uart.h"
#include "esp_log.h"

// ponytail: assumes a UART console (the esp32 default). A USB-Serial-JTAG
// console would need usb_serial_jtag_read_bytes instead.
#define CONSOLE_UART CONFIG_ESP_CONSOLE_UART_NUM

static const char *TAG = "console_input";

static command_source_t s_source;

static esp_err_t console_init(void *ctx)
{
    (void)ctx;
    // RX only; logging keeps writing to the console as before.
    esp_err_t err = uart_driver_install(CONSOLE_UART, 256, 0, 0, NULL, 0);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "press 'l' for lateral flow, 'c' for colorimetric");
    }
    return err;
}

static esp_err_t console_wait_request(void *ctx, strip_mode_t *out_mode)
{
    (void)ctx;
    for (;;) {
        uint8_t c;
        int n = uart_read_bytes(CONSOLE_UART, &c, 1, portMAX_DELAY);
        if (n < 0) {
            return ESP_FAIL;
        }
        switch (n == 1 ? c : 0) {
        case 'l': case 'L':
            *out_mode = STRIP_MODE_LATERAL_FLOW;
            return ESP_OK;
        case 'c': case 'C':
            *out_mode = STRIP_MODE_COLORIMETRIC;
            return ESP_OK;
        case 0: case '\r': case '\n':
            break;
        default:
            ESP_LOGI(TAG, "unknown key; press 'l' or 'c'");
        }
    }
}

command_source_t *console_input_create(void)
{
    s_source = (command_source_t){
        .init = console_init, .wait_request = console_wait_request, .ctx = NULL};
    return &s_source;
}
