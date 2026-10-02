# Composition root: the only component that knows about every layer.
idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES usecase domain ports
                                mock_sensor as7341 led_gpio console_input log_sink ble_mobiledetect
                                i2c_bus freertos_timebase
                                freertos log)
