set(WATCH_CORE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/../../../products/f411_watch/core")
set(WATCH_INPUT_SOURCE_DIR "${CMAKE_SOURCE_DIR}/../../../products/f411_watch/input")
set(WATCH_RUNTIME_SOURCE_DIR "${CMAKE_SOURCE_DIR}/../../../products/f411_watch/runtime")
set(WATCH_SENSOR_SOURCE_DIR "${CMAKE_SOURCE_DIR}/../../../products/f411_watch/sensors")
set(WATCH_STORAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/../../../products/f411_watch/storage")
set(WATCH_POWER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/../../../products/f411_watch/power")
set(WATCH_TIME_SOURCE_DIR "${CMAKE_SOURCE_DIR}/../../../products/f411_watch/time")
set(WATCH_TRANSPORT_SOURCE_DIR "${CMAKE_SOURCE_DIR}/../../../products/f411_watch/transport")
set(WATCH_LVGL_SOURCE_DIR "${CMAKE_SOURCE_DIR}/third_party/lvgl")
set(WATCH_LITTLEFS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/third_party/littlefs")
set(WATCH_TINYCRYPT_SOURCE_DIR "${CMAKE_SOURCE_DIR}/third_party/tinycrypt")

file(GLOB_RECURSE WATCH_LVGL_SOURCES CONFIGURE_DEPENDS
    "${WATCH_LVGL_SOURCE_DIR}/src/*.c"
)

add_library(watch_lvgl STATIC ${WATCH_LVGL_SOURCES})

target_include_directories(watch_lvgl
    SYSTEM PUBLIC
        ${WATCH_LVGL_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/ui
)

target_compile_definitions(watch_lvgl PUBLIC
    LV_CONF_INCLUDE_SIMPLE
    LV_LVGL_H_INCLUDE_SIMPLE
)

add_library(f411_watch_user OBJECT
    app/watch_app.c
    app/watch_diagnostic.c
    app/watch_fault_handlers.c
    app/watch_usb_diagnostic.c
    board/display/watch_lcd.c
    board/input/watch_cst816.c
    board/input/watch_input_hw.c
    board/power/watch_power.c
    board/sensors/watch_aht20_board.c
    board/sensors/watch_cw2015_board.c
    board/sensors/watch_max30102_board.c
    board/sensors/watch_lis2mdl_board.c
    board/sensors/watch_lsm6ds3_board.c
    board/sensors/watch_sensor_aggregate_board.c
    board/storage/watch_eeprom_probe_board.c
    board/storage/watch_littlefs_board.c
    board/storage/watch_ota_board.c
    board/storage/watch_w25q128_board.c
    board/time/watch_rtc_board.c
    board/bluetooth/watch_kt6368_board.c
    board/bluetooth/watch_kt6368_dma_irq.c
    board/usb/watch_usb_cdc.c
    ui/watch_ui.c
    ${CMAKE_SOURCE_DIR}/../../../products/f411_watch/ui/f411_watch_ui_gen.c
    ${CMAKE_SOURCE_DIR}/../../../products/f411_watch/ui/watch_page_lifecycle.c
    ${CMAKE_SOURCE_DIR}/../../../products/f411_watch/ui/screens/watchface/screen_watchface_gen.c
    ${CMAKE_SOURCE_DIR}/../../../products/f411_watch/ui/screens/launcher/screen_launcher_gen.c
    ${CMAKE_SOURCE_DIR}/../../../products/f411_watch/ui/screens/diagnostics/screen_diagnostics_gen.c
    ${CMAKE_SOURCE_DIR}/../../../products/f411_watch/ui/screens/resources/screen_resources_gen.c
    ${CMAKE_SOURCE_DIR}/../../../products/f411_watch/ui/screens/status/screen_status_gen.c
    ${CMAKE_SOURCE_DIR}/../../../products/f411_watch/ui/screens/settings/screen_settings_gen.c
    ${WATCH_CORE_SOURCE_DIR}/watch_core.c
    ${WATCH_INPUT_SOURCE_DIR}/watch_input.c
    ${WATCH_RUNTIME_SOURCE_DIR}/watch_runtime.c
    ${WATCH_SENSOR_SOURCE_DIR}/watch_lis2mdl.c
    ${WATCH_SENSOR_SOURCE_DIR}/watch_aht20.c
    ${WATCH_SENSOR_SOURCE_DIR}/watch_cw2015.c
    ${WATCH_SENSOR_SOURCE_DIR}/watch_max30102.c
    ${WATCH_SENSOR_SOURCE_DIR}/watch_lsm6ds3.c
    ${WATCH_SENSOR_SOURCE_DIR}/watch_lsm6ds3_sensor_hub.c
    ${WATCH_SENSOR_SOURCE_DIR}/watch_sensor_aggregate.c
    ${WATCH_STORAGE_SOURCE_DIR}/watch_eeprom_probe.c
    ${WATCH_STORAGE_SOURCE_DIR}/watch_littlefs.c
    ${WATCH_STORAGE_SOURCE_DIR}/watch_ota_metadata.c
    ${WATCH_STORAGE_SOURCE_DIR}/watch_ota_install.c
    ${WATCH_STORAGE_SOURCE_DIR}/watch_ota_package.c
    ${WATCH_STORAGE_SOURCE_DIR}/watch_w25q128.c
    ${WATCH_LITTLEFS_SOURCE_DIR}/lfs.c
    ${WATCH_LITTLEFS_SOURCE_DIR}/lfs_util.c
    ${WATCH_POWER_SOURCE_DIR}/watch_power_state.c
    ${WATCH_POWER_SOURCE_DIR}/watch_watchdog.c
    ${WATCH_TIME_SOURCE_DIR}/watch_time.c
    ${WATCH_TRANSPORT_SOURCE_DIR}/watch_resource_protocol.c
    ${WATCH_TRANSPORT_SOURCE_DIR}/watch_spp_transport.c
    ${WATCH_TINYCRYPT_SOURCE_DIR}/lib/source/sha256.c
    ${WATCH_TINYCRYPT_SOURCE_DIR}/lib/source/ecc.c
    ${WATCH_TINYCRYPT_SOURCE_DIR}/lib/source/ecc_dsa.c
    ${WATCH_TINYCRYPT_SOURCE_DIR}/lib/source/utils.c
)

target_include_directories(f411_watch_user
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${WATCH_CORE_SOURCE_DIR}
        ${WATCH_INPUT_SOURCE_DIR}
        ${WATCH_RUNTIME_SOURCE_DIR}
        ${WATCH_SENSOR_SOURCE_DIR}
        ${WATCH_STORAGE_SOURCE_DIR}
        ${WATCH_POWER_SOURCE_DIR}
        ${WATCH_TIME_SOURCE_DIR}
        ${WATCH_TRANSPORT_SOURCE_DIR}
        ${WATCH_LITTLEFS_SOURCE_DIR}
        ${WATCH_TINYCRYPT_SOURCE_DIR}/lib/include
        ${CMAKE_SOURCE_DIR}/../../../products/f411_watch/ui
)

target_link_libraries(f411_watch_user
    PUBLIC
        stm32cubemx
        watch_lvgl
)

add_custom_target(cppcheck
    COMMAND ${CMAKE_COMMAND}
        -DPROJECT_SOURCE_DIR=${CMAKE_SOURCE_DIR}
        -DCOMPILE_COMMANDS=${CMAKE_BINARY_DIR}/compile_commands.json
        -DCPPCHECK_BUILD_DIR=${CMAKE_BINARY_DIR}/cppcheck
        -DSUPPRESSIONS_FILE=${CMAKE_CURRENT_SOURCE_DIR}/../cmake/cppcheck.suppressions
        -DCPPCHECK_EXECUTABLE=${WATCH_CPPCHECK_EXECUTABLE}
        -P ${CMAKE_SOURCE_DIR}/cmake/run_cppcheck.cmake
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM
)

# Keep this list explicit. It is the only formatting boundary for this project
# and intentionally excludes CubeMX-generated and third-party sources.
add_custom_target(format
    COMMAND ${CMAKE_COMMAND}
        -DFORMAT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
        -DFORMAT_EXTRA_ROOT=${CMAKE_SOURCE_DIR}/bootloader
        -DFORMAT_CORE_ROOT=${WATCH_CORE_SOURCE_DIR}
        -DFORMAT_INPUT_ROOT=${WATCH_INPUT_SOURCE_DIR}
        -DFORMAT_RUNTIME_ROOT=${WATCH_RUNTIME_SOURCE_DIR}
        -DFORMAT_SENSOR_ROOT=${WATCH_SENSOR_SOURCE_DIR}
        -DFORMAT_STORAGE_ROOT=${WATCH_STORAGE_SOURCE_DIR}
        -DFORMAT_POWER_ROOT=${WATCH_POWER_SOURCE_DIR}
        -DFORMAT_TIME_ROOT=${WATCH_TIME_SOURCE_DIR}
        -DFORMAT_TRANSPORT_ROOT=${WATCH_TRANSPORT_SOURCE_DIR}
        -DFORMAT_MODE=fix
        -DCLANG_FORMAT_EXECUTABLE=${WATCH_CLANG_FORMAT_EXECUTABLE}
        -P ${CMAKE_SOURCE_DIR}/cmake/run_clang_format.cmake
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM
)

add_custom_target(format-check
    COMMAND ${CMAKE_COMMAND}
        -DFORMAT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
        -DFORMAT_EXTRA_ROOT=${CMAKE_SOURCE_DIR}/bootloader
        -DFORMAT_CORE_ROOT=${WATCH_CORE_SOURCE_DIR}
        -DFORMAT_INPUT_ROOT=${WATCH_INPUT_SOURCE_DIR}
        -DFORMAT_RUNTIME_ROOT=${WATCH_RUNTIME_SOURCE_DIR}
        -DFORMAT_SENSOR_ROOT=${WATCH_SENSOR_SOURCE_DIR}
        -DFORMAT_STORAGE_ROOT=${WATCH_STORAGE_SOURCE_DIR}
        -DFORMAT_POWER_ROOT=${WATCH_POWER_SOURCE_DIR}
        -DFORMAT_TIME_ROOT=${WATCH_TIME_SOURCE_DIR}
        -DFORMAT_TRANSPORT_ROOT=${WATCH_TRANSPORT_SOURCE_DIR}
        -DFORMAT_MODE=check
        -DCLANG_FORMAT_EXECUTABLE=${WATCH_CLANG_FORMAT_EXECUTABLE}
        -P ${CMAKE_SOURCE_DIR}/cmake/run_clang_format.cmake
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM
)
