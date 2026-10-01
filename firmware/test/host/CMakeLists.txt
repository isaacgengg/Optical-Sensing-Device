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
    fakes)
target_compile_options(core_host PRIVATE -Wall -Wextra -Werror)

enable_testing()
foreach(t test_strip_analysis test_measurement test_mock_sensor test_mobiledetect_protocol)
    add_executable(${t} ${t}.c)
    target_link_libraries(${t} PRIVATE core_host unity)
    target_compile_options(${t} PRIVATE -Wall -Wextra -Werror)
    add_test(NAME ${t} COMMAND ${t})
endforeach()
