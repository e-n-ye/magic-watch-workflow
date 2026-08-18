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
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/sensors/watch_sensor_aggregate.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/time/watch_time.c
    )

    target_include_directories(watch_core_input_tests
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/core
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/input
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/sensors
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/time
    )

    add_test(NAME watch_core_input COMMAND watch_core_input_tests)

    add_executable(watch_core_diagnostic_tests
        ${CMAKE_CURRENT_LIST_DIR}/watch_core_diagnostic_test.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/core/watch_core.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/sensors/watch_sensor_aggregate.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/time/watch_time.c
    )

    target_compile_definitions(watch_core_diagnostic_tests PRIVATE WATCH_DIAGNOSTIC=1)
    target_include_directories(watch_core_diagnostic_tests
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/core
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/sensors
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
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/sensors
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

    add_executable(watch_cw2015_tests
        ${CMAKE_CURRENT_LIST_DIR}/watch_cw2015_test.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/sensors/watch_cw2015.c
    )

    target_include_directories(watch_cw2015_tests
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/sensors
    )

    add_test(NAME watch_cw2015 COMMAND watch_cw2015_tests)

    add_executable(watch_sensor_aggregate_tests
        ${CMAKE_CURRENT_LIST_DIR}/watch_sensor_aggregate_test.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/sensors/watch_sensor_aggregate.c
    )

    target_include_directories(watch_sensor_aggregate_tests
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/sensors
    )

    add_test(NAME watch_sensor_aggregate COMMAND watch_sensor_aggregate_tests)

    add_executable(watch_eeprom_probe_tests
        ${CMAKE_CURRENT_LIST_DIR}/watch_eeprom_probe_test.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/storage/watch_eeprom_probe.c
    )

    target_include_directories(watch_eeprom_probe_tests
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/storage
    )

    add_test(NAME watch_eeprom_probe COMMAND watch_eeprom_probe_tests)

    add_executable(watch_w25q128_tests
        ${CMAKE_CURRENT_LIST_DIR}/watch_w25q128_test.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/storage/watch_w25q128.c
    )

    target_include_directories(watch_w25q128_tests
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/storage
    )

    add_test(NAME watch_w25q128 COMMAND watch_w25q128_tests)

    add_executable(watch_littlefs_tests
        ${CMAKE_CURRENT_LIST_DIR}/watch_littlefs_test.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/storage/watch_littlefs.c
        ${CMAKE_CURRENT_LIST_DIR}/../firmware/stm32/f411_watch/third_party/littlefs/lfs.c
        ${CMAKE_CURRENT_LIST_DIR}/../firmware/stm32/f411_watch/third_party/littlefs/lfs_util.c
    )

    target_include_directories(watch_littlefs_tests
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/storage
            ${CMAKE_CURRENT_LIST_DIR}/../firmware/stm32/f411_watch/third_party/littlefs
    )

    add_test(NAME watch_littlefs COMMAND watch_littlefs_tests)

    add_executable(watch_resource_protocol_tests
        ${CMAKE_CURRENT_LIST_DIR}/watch_resource_protocol_test.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/transport/watch_resource_protocol.c
        ${CMAKE_CURRENT_LIST_DIR}/../firmware/stm32/f411_watch/third_party/tinycrypt/lib/source/sha256.c
        ${CMAKE_CURRENT_LIST_DIR}/../firmware/stm32/f411_watch/third_party/tinycrypt/lib/source/utils.c
    )

    target_include_directories(watch_resource_protocol_tests
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/transport
            ${CMAKE_CURRENT_LIST_DIR}/../firmware/stm32/f411_watch/third_party/tinycrypt/lib/include
    )

    add_test(NAME watch_resource_protocol COMMAND watch_resource_protocol_tests)

    add_executable(watch_spp_transport_tests
        ${CMAKE_CURRENT_LIST_DIR}/watch_spp_transport_test.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/transport/watch_spp_transport.c
    )

    target_include_directories(watch_spp_transport_tests
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/transport
    )

    add_test(NAME watch_spp_transport COMMAND watch_spp_transport_tests)

    add_executable(watch_max30102_tests
        ${CMAKE_CURRENT_LIST_DIR}/watch_max30102_test.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/sensors/watch_max30102.c
    )

    target_include_directories(watch_max30102_tests
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/sensors
    )

    add_test(NAME watch_max30102 COMMAND watch_max30102_tests)

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
