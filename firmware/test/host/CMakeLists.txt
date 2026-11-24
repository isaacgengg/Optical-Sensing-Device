# Standalone host build of the core (not an ESP-IDF project).
cmake_minimum_required(VERSION 3.16)
project(detectachem_host_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG        v2.6.1)
FetchContent_MakeAvailable(unity)

set(FW ${CMAKE_CURRENT_SOURCE_DIR}/../..)

# shim/ comes first so "esp_err.h" resolves to the host stand-in.
add_library(core_host STATIC
    ${FW}/core/domain/strip_analysis.c
    ${FW}/core/usecase/measurement.c
    ${FW}/adapters/mock_sensor/mock_sensor.c
    ${FW}/adapters/mock_sensor/mock_data.c
    ${FW}/adapters/mobiledetect_protocol/mobiledetect_protocol.c
    ${FW}/adapters/tcs3448/tcs3448.c
    ${FW}/adapters/as7343/as7343.c
    fakes/fake_i2c_bus.c
    fakes/fake_illuminator.c
    fakes/fake_timebase.c
    fakes/fake_sink.c)
target_include_directories(core_host PUBLIC
    shim
    ${FW}/core/domain/include
    ${FW}/core/ports/include
    ${FW}/core/usecase/include
    ${FW}/adapters/mock_sensor/include
    ${FW}/adapters/mobiledetect_protocol/include
    ${FW}/adapters/tcs3448/include
    ${FW}/adapters/as7343/include
    ${FW}/platform/i2c_bus/include
    fakes)
target_link_libraries(core_host PUBLIC m)  # sqrtf
target_compile_options(core_host PRIVATE -Wall -Wextra -Werror)

enable_testing()
# Tests mirror the architecture: <layer>/test_<name>.c
foreach(src
        domain/test_strip_analysis.c
        domain/test_color_refs.c
        domain/test_lfa_refs.c
        usecase/test_measurement.c
        adapters/test_mock_sensor.c
        adapters/test_mobiledetect_protocol.c
        adapters/test_tcs3448.c
        adapters/test_as7343.c
        adapters/test_dual_sensors.c)
    get_filename_component(t ${src} NAME_WE)
    add_executable(${t} ${src})
    target_link_libraries(${t} PRIVATE core_host unity)
    target_compile_options(${t} PRIVATE -Wall -Wextra -Werror)
    if(t STREQUAL "test_color_refs" OR t STREQUAL "test_lfa_refs")
        target_include_directories(${t} PRIVATE ${FW}/main)
    endif()
    if(t STREQUAL "test_lfa_refs")
        target_compile_definitions(${t} PRIVATE
            CONFIG_DC_SENSOR_MOCK=0 CONFIG_DC_SENSOR_TCS3448=1
            CONFIG_DC_LFA_AS7343=1 CONFIG_DC_SENSOR_LEDS=1 CONFIG_DC_SENSOR_LED_MA=50)
    endif()
    add_test(NAME ${t} COMMAND ${t})
endforeach()

# Measured refs must not silently apply to another light setting or sensor setup.
function(add_lfa_profile_test name mock tcs dual leds ma)
    add_executable(${name} domain/test_lfa_profile_selection.c)
    target_link_libraries(${name} PRIVATE core_host unity)
    target_include_directories(${name} PRIVATE ${FW}/main)
    target_compile_options(${name} PRIVATE -Wall -Wextra -Werror)
    target_compile_definitions(${name} PRIVATE
        CONFIG_DC_SENSOR_MOCK=${mock} CONFIG_DC_SENSOR_TCS3448=${tcs}
        CONFIG_DC_LFA_AS7343=${dual} CONFIG_DC_SENSOR_LEDS=${leds}
        CONFIG_DC_SENSOR_LED_MA=${ma})
    add_test(NAME ${name} COMMAND ${name})
endfunction()
add_lfa_profile_test(test_lfa_profile_leds_off 0 1 1 0 50)
add_lfa_profile_test(test_lfa_profile_leds_20ma 0 1 1 1 20)
add_lfa_profile_test(test_lfa_profile_single_sensor 0 1 0 1 50)
add_lfa_profile_test(test_lfa_profile_mock 1 0 0 0 0)
