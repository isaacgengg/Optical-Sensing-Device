idf_component_register(SRCS "console_input.c"
                       INCLUDE_DIRS "include"
                       REQUIRES ports domain esp_driver_uart log)
