#include "watch_app.h"

#include "watch_diagnostic.h"
#include "board/display/watch_lcd.h"

void watch_app_init(void)
{
    watch_diagnostic_capsule_t capsule;

    watch_lcd_init();
    watch_lcd_backlight_on();

    if (watch_diagnostic_get(&capsule)) {
        watch_lcd_show_diagnostic_pattern(capsule.reason);
        watch_diagnostic_clear();
        return;
    }

    watch_lcd_show_bringup_pattern();
}
