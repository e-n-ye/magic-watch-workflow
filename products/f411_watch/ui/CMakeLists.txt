# Generated-style CMake entry for the UI exported from LVGL Pro Editor.
# Keep user-owned additions in user_config.cmake.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.22)
    project(f411_watch_ui LANGUAGES C)
endif()

if(DEFINED LVGL_COMPONENT_LIB_NAME)
    set(LIB_NAME ${LVGL_COMPONENT_LIB_NAME})
else()
    set(LIB_NAME f411_watch_ui)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/user_config.cmake)
set(LV_EDITOR_USER_SOURCES ${LV_EDITOR_PROJECT_SOURCES})
set(LV_EDITOR_PROJECT_SOURCES)
include(${CMAKE_CURRENT_LIST_DIR}/component_lib_list_gen.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/file_list_gen.cmake)

add_library(${LIB_NAME} ${LV_EDITOR_PROJECT_SOURCES})

if(LV_EDITOR_USER_SOURCES)
    add_library(${LIB_NAME}_user OBJECT ${LV_EDITOR_USER_SOURCES})
    if(DEFINED LV_EDITOR_COMPONENT_LIB_LIST)
        target_link_libraries(${LIB_NAME}_user PUBLIC ${LV_EDITOR_COMPONENT_LIB_LIST})
    endif()
    target_link_libraries(${LIB_NAME} PUBLIC $<TARGET_OBJECTS:${LIB_NAME}_user>)
endif()

if(DEFINED LV_EDITOR_COMPONENT_LIB_LIST)
    target_link_libraries(${LIB_NAME} PUBLIC ${LV_EDITOR_COMPONENT_LIB_LIST})
endif()

if(TARGET lvgl)
    target_link_libraries(${LIB_NAME} PUBLIC lvgl)
endif()

target_include_directories(${LIB_NAME}
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
)
