// TODO(team): on/off only. If intensity control matters, drive the LED with
// PWM via LEDC and map intensity_pct to duty cycle.

#include "led_gpio.h"
#include "driver/gpio.h"

typedef struct {
    gpio_num_t gpio;
} led_ctx_t;

static led_ctx_t s_ctx;
static illuminator_t s_light;

static esp_err_t led_init(void *ctx)
{
    led_ctx_t *led = ctx;
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << led->gpio,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&cfg);
    if (err != ESP_OK) {
        return err;
    }
    return gpio_set_level(led->gpio, 0);
}

static esp_err_t led_set(void *ctx, bool on, uint8_t intensity_pct)
{
    led_ctx_t *led = ctx;
    return gpio_set_level(led->gpio, (on && intensity_pct > 0) ? 1 : 0);
}

illuminator_t *led_gpio_create(int gpio_num)
{
    s_ctx = (led_ctx_t){.gpio = (gpio_num_t)gpio_num};
    s_light = (illuminator_t){.init = led_init, .set = led_set, .ctx = &s_ctx};
    return &s_light;
}
