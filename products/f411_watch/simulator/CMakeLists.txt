cmake_minimum_required(VERSION 3.22)

project(magic_watch_f411_simulator LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)

set(WATCH_LVGL_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../firmware/stm32/f411_watch/third_party/lvgl")
set(WATCH_UI_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../ui")
set(WATCH_CORE_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../core")

file(GLOB_RECURSE WATCH_LVGL_SOURCES CONFIGURE_DEPENDS
    "${WATCH_LVGL_SOURCE_DIR}/src/*.c"
)

add_library(watch_ui_lvgl STATIC ${WATCH_LVGL_SOURCES})
target_include_directories(watch_ui_lvgl
    SYSTEM PUBLIC
        ${WATCH_LVGL_SOURCE_DIR}
        ${WATCH_LVGL_SOURCE_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}
)
target_compile_definitions(watch_ui_lvgl PUBLIC
    LV_CONF_INCLUDE_SIMPLE
    LV_LVGL_H_INCLUDE_SIMPLE
)

set(LVGL_COMPONENT_LIB_NAME watch_ui_generated)
add_subdirectory(${WATCH_UI_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/ui)
target_link_libraries(watch_ui_generated PUBLIC watch_ui_lvgl)
target_include_directories(watch_ui_generated PUBLIC ${WATCH_UI_SOURCE_DIR})

add_executable(watch_ui_simulator
    main.c
    ${WATCH_CORE_SOURCE_DIR}/watch_core.c
)
target_include_directories(watch_ui_simulator
    PRIVATE
        ${WATCH_CORE_SOURCE_DIR}
        ${WATCH_UI_SOURCE_DIR}
)
target_compile_definitions(watch_ui_simulator PRIVATE
    LV_CONF_INCLUDE_SIMPLE
    LV_LVGL_H_INCLUDE_SIMPLE
)
target_link_libraries(watch_ui_simulator PRIVATE
    watch_ui_generated
    watch_ui_lvgl
)

if(WIN32)
    target_link_libraries(watch_ui_simulator PRIVATE
        gdi32
        imm32
        ole32
        user32
    )
endif()

if(BUILD_TESTING)
    add_test(NAME f411_watch_ui_smoke COMMAND watch_ui_simulator --smoke)
    set_tests_properties(f411_watch_ui_smoke PROPERTIES
        PASS_REGULAR_EXPRESSION "watch_ui_smoke: PASS"
    )
endif()
