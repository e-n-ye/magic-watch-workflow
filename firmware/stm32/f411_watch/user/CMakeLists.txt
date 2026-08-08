add_library(f411_watch_user OBJECT
    app/watch_app.c
    board/display/watch_lcd.c
    board/power/watch_power.c
)

target_include_directories(f411_watch_user
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(f411_watch_user
    PUBLIC
        stm32cubemx
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
        -DFORMAT_MODE=check
        -DCLANG_FORMAT_EXECUTABLE=${WATCH_CLANG_FORMAT_EXECUTABLE}
        -P ${CMAKE_SOURCE_DIR}/cmake/run_clang_format.cmake
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM
)
