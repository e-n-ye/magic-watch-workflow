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
