idf_component_register(SRCS "ble_mobiledetect.c"
                       INCLUDE_DIRS "include"
                       REQUIRES ports mobiledetect_protocol bt nvs_flash log)
