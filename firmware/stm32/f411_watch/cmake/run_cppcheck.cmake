if(NOT DEFINED PROJECT_SOURCE_DIR OR PROJECT_SOURCE_DIR STREQUAL "")
    message(FATAL_ERROR "PROJECT_SOURCE_DIR is required")
endif()

if(NOT DEFINED COMPILE_COMMANDS OR NOT EXISTS "${COMPILE_COMMANDS}")
    message(FATAL_ERROR "CMake compilation database was not found: ${COMPILE_COMMANDS}")
endif()

if(NOT DEFINED SUPPRESSIONS_FILE OR NOT EXISTS "${SUPPRESSIONS_FILE}")
    message(FATAL_ERROR "Cppcheck suppressions file was not found: ${SUPPRESSIONS_FILE}")
endif()

if(NOT DEFINED CPPCHECK_EXECUTABLE OR CPPCHECK_EXECUTABLE STREQUAL "")
    unset(CPPCHECK_EXECUTABLE CACHE)
    unset(CPPCHECK_EXECUTABLE)
    find_program(CPPCHECK_EXECUTABLE NAMES cppcheck)
endif()

if((NOT CPPCHECK_EXECUTABLE OR CPPCHECK_EXECUTABLE MATCHES "-NOTFOUND$") AND WIN32)
    foreach(candidate
        "C:/Program Files/Cppcheck/cppcheck.exe"
        "C:/Program Files (x86)/Cppcheck/cppcheck.exe"
        "$ENV{ProgramFiles}/Cppcheck/cppcheck.exe"
    )
        if(EXISTS "${candidate}")
            set(CPPCHECK_EXECUTABLE "${candidate}")
            break()
        endif()
    endforeach()
endif()

if(NOT CPPCHECK_EXECUTABLE OR CPPCHECK_EXECUTABLE MATCHES "-NOTFOUND$")
    message(FATAL_ERROR "cppcheck was not found. Install Cppcheck 2.21.0 or set WATCH_CPPCHECK_EXECUTABLE.")
endif()

file(MAKE_DIRECTORY "${CPPCHECK_BUILD_DIR}")

execute_process(
    COMMAND "${CPPCHECK_EXECUTABLE}"
        "--project=${COMPILE_COMMANDS}"
        "--file-filter=user/**"
        "--file-filter=products/f411_watch/core/**"
        "--file-filter=products/f411_watch/input/**"
        "--file-filter=products/f411_watch/runtime/**"
        "--file-filter=bootloader/**"
        "--cppcheck-build-dir=${CPPCHECK_BUILD_DIR}"
        "--platform=arm32-wchar_t4"
        "--check-level=normal"
        "--enable=warning,style,performance,portability"
        "--inline-suppr"
        "--suppressions-list=${SUPPRESSIONS_FILE}"
        "--suppress=missingIncludeSystem"
        "--template=gcc"
        "--error-exitcode=1"
        "--quiet"
        "-D__GNUC__"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMAND_ECHO STDOUT
    RESULT_VARIABLE CPPCHECK_RESULT
)

if(NOT CPPCHECK_RESULT EQUAL 0)
    message(FATAL_ERROR "cppcheck failed with exit code ${CPPCHECK_RESULT}")
endif()
