cmake_minimum_required(VERSION 3.16)

# Components live in layer folders instead of a flat components/ directory,
# so the folder tree mirrors the architecture.
set(EXTRA_COMPONENT_DIRS
    core/domain core/ports core/usecase
    adapters/mock_sensor adapters/as7341 adapters/led_gpio adapters/console_input
    adapters/log_sink adapters/mobiledetect_protocol adapters/ble_mobiledetect
    platform/i2c_bus platform/freertos_timebase)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(detectachem_firmware)
