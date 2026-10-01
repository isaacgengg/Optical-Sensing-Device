idf_component_register(SRCS "led_gpio.c"
                       INCLUDE_DIRS "include"
                       REQUIRES ports esp_driver_gpio)
