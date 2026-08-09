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
    )

    target_include_directories(watch_core_input_tests
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/core
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/input
    )

    add_test(NAME watch_core_input COMMAND watch_core_input_tests)

    add_executable(watch_runtime_tests
        ${CMAKE_CURRENT_LIST_DIR}/watch_runtime_test.c
        ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/runtime/watch_runtime.c
    )

    target_include_directories(watch_runtime_tests
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/../products/f411_watch/runtime
    )

    add_test(NAME watch_runtime COMMAND watch_runtime_tests)
endif()
