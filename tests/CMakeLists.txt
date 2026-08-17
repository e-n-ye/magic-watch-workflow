cmake_minimum_required(VERSION 3.22)

project(magic_watch_host_tests LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(CTest)

if(BUILD_TESTING)
    add_executable(watch_core_input_tests
        ${CMAKE_CURRENT_LIST_DIR}/watch_core_test.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/core/watch_core.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/input/watch_input.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/time/watch_time.c
    )

    target_include_directories(watch_core_input_tests
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/core
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/input
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/time
    )

    add_test(NAME watch_core_input COMMAND watch_core_input_tests)

    add_executable(watch_core_diagnostic_tests
        ${CMAKE_CURRENT_LIST_DIR}/watch_core_diagnostic_test.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/core/watch_core.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/time/watch_time.c
    )

    target_compile_definitions(watch_core_diagnostic_tests PRIVATE WATCH_DIAGNOSTIC=1)
    target_include_directories(watch_core_diagnostic_tests
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/core
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/time
    )

    add_test(NAME watch_core_diagnostic_preset COMMAND watch_core_diagnostic_tests)

    add_executable(watch_time_tests
        ${CMAKE_CURRENT_LIST_DIR}/watch_time_test.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/time/watch_time.c
    )

    target_include_directories(watch_time_tests
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/time
    )

    add_test(NAME watch_time COMMAND watch_time_tests)

    add_executable(watch_runtime_tests
        ${CMAKE_CURRENT_LIST_DIR}/watch_runtime_test.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/runtime/watch_runtime.c
    )

    target_include_directories(watch_runtime_tests
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/runtime
    )

    add_test(NAME watch_runtime COMMAND watch_runtime_tests)

    add_executable(watch_lsm6ds3_tests
        ${CMAKE_CURRENT_LIST_DIR}/watch_lsm6ds3_test.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/sensors/watch_lsm6ds3.c
    )

    target_include_directories(watch_lsm6ds3_tests
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/sensors
    )

    add_test(NAME watch_lsm6ds3 COMMAND watch_lsm6ds3_tests)

    add_executable(watch_lis2mdl_tests
        ${CMAKE_CURRENT_LIST_DIR}/watch_lis2mdl_test.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/sensors/watch_lis2mdl.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/sensors/watch_lsm6ds3_sensor_hub.c
    )

    target_include_directories(watch_lis2mdl_tests
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/sensors
    )

    add_test(NAME watch_lis2mdl COMMAND watch_lis2mdl_tests)

    add_executable(watch_aht20_tests
        ${CMAKE_CURRENT_LIST_DIR}/watch_aht20_test.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/sensors/watch_aht20.c
    )

    target_include_directories(watch_aht20_tests
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/sensors
    )

    add_test(NAME watch_aht20 COMMAND watch_aht20_tests)

    add_executable(watch_power_tests
        ${CMAKE_CURRENT_LIST_DIR}/watch_power_test.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/power/watch_power_state.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/power/watch_watchdog.c
    )

    target_include_directories(watch_power_tests
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/power
    )

    add_test(NAME watch_power COMMAND watch_power_tests)
endif()
