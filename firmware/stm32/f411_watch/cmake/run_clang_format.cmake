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
