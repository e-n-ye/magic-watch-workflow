# Manual Editor-export project sources are listed in file_list_gen.cmake.
# Keep this hook for future user-owned components without editing generated lists.
list(APPEND LV_EDITOR_PROJECT_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/watch_page_lifecycle.c
)
