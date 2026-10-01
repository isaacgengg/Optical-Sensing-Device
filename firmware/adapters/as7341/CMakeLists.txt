# PLACEHOLDER: sensor part not confirmed. Rename/replace once it is.
idf_component_register(SRCS "as7341.c"
                       INCLUDE_DIRS "include"
                       REQUIRES ports i2c_bus)
