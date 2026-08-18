if(NOT DEFINED FORMAT_ROOT OR FORMAT_ROOT STREQUAL "")
    message(FATAL_ERROR "FORMAT_ROOT must point to the F411 user directory.")
endif()

if(NOT DEFINED FORMAT_MODE OR NOT FORMAT_MODE MATCHES "^(fix|check)$")
    message(FATAL_ERROR "FORMAT_MODE must be either fix or check.")
endif()

if(NOT DEFINED CLANG_FORMAT_EXECUTABLE OR CLANG_FORMAT_EXECUTABLE STREQUAL "")
    unset(CLANG_FORMAT_EXECUTABLE CACHE)
    unset(CLANG_FORMAT_EXECUTABLE)
    find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format)
endif()

if(NOT CLANG_FORMAT_EXECUTABLE)
    message(FATAL_ERROR
        "clang-format was not found. Install the pinned project tool or set "
        "-DWATCH_CLANG_FORMAT_EXECUTABLE=<path> when configuring CMake."
    )
endif()

set(FORMAT_FILES
    "${FORMAT_ROOT}/app/watch_app.c"
    "${FORMAT_ROOT}/app/watch_app.h"
    "${FORMAT_ROOT}/board/display/watch_lcd.c"
    "${FORMAT_ROOT}/board/display/watch_lcd.h"
    "${FORMAT_ROOT}/board/input/watch_cst816.c"
    "${FORMAT_ROOT}/board/input/watch_cst816.h"
    "${FORMAT_ROOT}/board/input/watch_input_hw.c"
    "${FORMAT_ROOT}/board/input/watch_input_hw.h"
    "${FORMAT_ROOT}/board/power/watch_power.c"
    "${FORMAT_ROOT}/board/power/watch_power.h"
    "${FORMAT_ROOT}/board/sensors/watch_aht20_board.c"
    "${FORMAT_ROOT}/board/sensors/watch_aht20_board.h"
    "${FORMAT_ROOT}/board/sensors/watch_cw2015_board.c"
    "${FORMAT_ROOT}/board/sensors/watch_cw2015_board.h"
    "${FORMAT_ROOT}/board/sensors/watch_max30102_board.c"
    "${FORMAT_ROOT}/board/sensors/watch_max30102_board.h"
    "${FORMAT_ROOT}/board/sensors/watch_lis2mdl_board.c"
    "${FORMAT_ROOT}/board/sensors/watch_lis2mdl_board.h"
    "${FORMAT_ROOT}/board/sensors/watch_lsm6ds3_board.c"
    "${FORMAT_ROOT}/board/sensors/watch_lsm6ds3_board.h"
    "${FORMAT_ROOT}/board/sensors/watch_sensor_aggregate_board.c"
    "${FORMAT_ROOT}/board/sensors/watch_sensor_aggregate_board.h"
    "${FORMAT_ROOT}/board/storage/watch_eeprom_probe_board.c"
    "${FORMAT_ROOT}/board/storage/watch_eeprom_probe_board.h"
    "${FORMAT_ROOT}/board/storage/watch_littlefs_board.c"
    "${FORMAT_ROOT}/board/storage/watch_littlefs_board.h"
    "${FORMAT_ROOT}/board/storage/watch_w25q128_board.c"
    "${FORMAT_ROOT}/board/storage/watch_w25q128_board.h"
    "${FORMAT_ROOT}/board/time/watch_rtc_board.c"
    "${FORMAT_ROOT}/board/time/watch_rtc_board.h"
    "${FORMAT_ROOT}/config/user_config.h"
)

if(DEFINED FORMAT_EXTRA_ROOT AND NOT FORMAT_EXTRA_ROOT STREQUAL "")
    list(APPEND FORMAT_FILES
        "${FORMAT_EXTRA_ROOT}/main.c"
        "${FORMAT_EXTRA_ROOT}/system.c"
    )
endif()

if(DEFINED FORMAT_CORE_ROOT AND NOT FORMAT_CORE_ROOT STREQUAL "")
    list(APPEND FORMAT_FILES
        "${FORMAT_CORE_ROOT}/watch_core.c"
        "${FORMAT_CORE_ROOT}/watch_core.h"
    )
endif()

if(DEFINED FORMAT_INPUT_ROOT AND NOT FORMAT_INPUT_ROOT STREQUAL "")
    list(APPEND FORMAT_FILES
        "${FORMAT_INPUT_ROOT}/watch_input.c"
        "${FORMAT_INPUT_ROOT}/watch_input.h"
    )
endif()

if(DEFINED FORMAT_RUNTIME_ROOT AND NOT FORMAT_RUNTIME_ROOT STREQUAL "")
    list(APPEND FORMAT_FILES
        "${FORMAT_RUNTIME_ROOT}/watch_runtime.c"
        "${FORMAT_RUNTIME_ROOT}/watch_runtime.h"
    )
endif()

if(DEFINED FORMAT_SENSOR_ROOT AND NOT FORMAT_SENSOR_ROOT STREQUAL "")
    list(APPEND FORMAT_FILES
        "${FORMAT_SENSOR_ROOT}/watch_lis2mdl.c"
        "${FORMAT_SENSOR_ROOT}/watch_lis2mdl.h"
        "${FORMAT_SENSOR_ROOT}/watch_aht20.c"
        "${FORMAT_SENSOR_ROOT}/watch_aht20.h"
        "${FORMAT_SENSOR_ROOT}/watch_cw2015.c"
        "${FORMAT_SENSOR_ROOT}/watch_cw2015.h"
        "${FORMAT_SENSOR_ROOT}/watch_max30102.c"
        "${FORMAT_SENSOR_ROOT}/watch_max30102.h"
        "${FORMAT_SENSOR_ROOT}/watch_lsm6ds3.c"
        "${FORMAT_SENSOR_ROOT}/watch_lsm6ds3.h"
        "${FORMAT_SENSOR_ROOT}/watch_lsm6ds3_sensor_hub.c"
        "${FORMAT_SENSOR_ROOT}/watch_lsm6ds3_sensor_hub.h"
        "${FORMAT_SENSOR_ROOT}/watch_sensor_aggregate.c"
        "${FORMAT_SENSOR_ROOT}/watch_sensor_aggregate.h"
    )
endif()

if(DEFINED FORMAT_POWER_ROOT AND NOT FORMAT_POWER_ROOT STREQUAL "")
    list(APPEND FORMAT_FILES
        "${FORMAT_POWER_ROOT}/watch_power_state.c"
        "${FORMAT_POWER_ROOT}/watch_power_state.h"
        "${FORMAT_POWER_ROOT}/watch_watchdog.c"
        "${FORMAT_POWER_ROOT}/watch_watchdog.h"
    )
endif()

if(DEFINED FORMAT_TIME_ROOT AND NOT FORMAT_TIME_ROOT STREQUAL "")
    list(APPEND FORMAT_FILES
        "${FORMAT_TIME_ROOT}/watch_time.c"
        "${FORMAT_TIME_ROOT}/watch_time.h"
    )
endif()

if(DEFINED FORMAT_STORAGE_ROOT AND NOT FORMAT_STORAGE_ROOT STREQUAL "")
    list(APPEND FORMAT_FILES
        "${FORMAT_STORAGE_ROOT}/watch_eeprom_probe.c"
        "${FORMAT_STORAGE_ROOT}/watch_eeprom_probe.h"
        "${FORMAT_STORAGE_ROOT}/watch_littlefs.c"
        "${FORMAT_STORAGE_ROOT}/watch_littlefs.h"
        "${FORMAT_STORAGE_ROOT}/watch_w25_partitions.h"
        "${FORMAT_STORAGE_ROOT}/watch_w25q128.c"
        "${FORMAT_STORAGE_ROOT}/watch_w25q128.h"
    )
endif()

foreach(FORMAT_FILE IN LISTS FORMAT_FILES)
    if(NOT EXISTS "${FORMAT_FILE}")
        message(FATAL_ERROR "Formatting whitelist file does not exist: ${FORMAT_FILE}")
    endif()

    set(FORMAT_ARGUMENTS --style=file)
    if(FORMAT_MODE STREQUAL "fix")
        list(APPEND FORMAT_ARGUMENTS -i)
    else()
        list(APPEND FORMAT_ARGUMENTS --dry-run --Werror)
    endif()

    execute_process(
        COMMAND "${CLANG_FORMAT_EXECUTABLE}" ${FORMAT_ARGUMENTS} "${FORMAT_FILE}"
        WORKING_DIRECTORY "${FORMAT_ROOT}"
        RESULT_VARIABLE FORMAT_RESULT
        OUTPUT_VARIABLE FORMAT_OUTPUT
        ERROR_VARIABLE FORMAT_ERROR
    )

    if(NOT FORMAT_RESULT EQUAL 0)
        message(STATUS "${FORMAT_OUTPUT}")
        message(STATUS "${FORMAT_ERROR}")
        message(FATAL_ERROR "clang-format ${FORMAT_MODE} failed for ${FORMAT_FILE}")
    endif()
endforeach()
