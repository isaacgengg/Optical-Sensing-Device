// LED illuminator on a single GPIO (on/off only).
#pragma once
#include "illuminator.h"

illuminator_t *led_gpio_create(int gpio_num);
