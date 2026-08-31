# Manual Editor-export project sources are listed in file_list_gen.cmake.
# The Editor compiles this hook as a separate object library, so give it the
# sibling core module include roots needed by watch_core.h. Directory-scoped
# options apply to both generated and user targets created below.
add_compile_options(
    "-I${CMAKE_CURRENT_LIST_DIR}/../sensors"
    "-I${CMAKE_CURRENT_LIST_DIR}/../time"
)

# Keep this hook for future user-owned components without editing generated lists.
list(APPEND LV_EDITOR_PROJECT_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/watch_ui_theme.c
    ${CMAKE_CURRENT_LIST_DIR}/watch_page_lifecycle.c
)
