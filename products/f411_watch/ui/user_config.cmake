# Manual Editor-export project sources are listed in file_list_gen.cmake.
# The Editor compiles this hook as a separate object library, so give it the
# sibling core module include roots needed by watch_core.h.
add_compile_options(
    "-I${CMAKE_CURRENT_LIST_DIR}/../sensors"
    "-I${CMAKE_CURRENT_LIST_DIR}/../time"
)

# Keep this hook for future user-owned components without editing generated lists.
# The LVGL Pro Editor preview builds only the declarative UI and does not have
# the firmware/service include graph required by these application-layer files.
if(NOT DEFINED LVED_ENV)
    list(APPEND LV_EDITOR_PROJECT_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/watch_ui_theme.c
        ${CMAKE_CURRENT_LIST_DIR}/watch_launcher_interaction.c
        ${CMAKE_CURRENT_LIST_DIR}/watch_page_lifecycle.c
    )
endif()
