#include "watch_app.h"

#include "board/display/watch_lcd.h"

void watch_app_init(void)
{
    watch_lcd_init();
    watch_lcd_backlight_on();
    watch_lcd_show_bringup_pattern();
}
