#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "f411_watch_ui.h"
#include "lvgl.h"
#include "watch_core.h"

#if defined(_WIN32)
#include "drivers/windows/lv_windows_display.h"
#endif

#define WATCH_SIMULATOR_WIDTH 240
#define WATCH_SIMULATOR_HEIGHT 280
#define WATCH_SIMULATOR_BUFFER_LINES 20

static uint16_t s_draw_buffer[WATCH_SIMULATOR_WIDTH * WATCH_SIMULATOR_BUFFER_LINES];

static void watch_simulator_flush(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
    (void)area;
    (void)px_map;
    lv_display_flush_ready(display);
}

static lv_display_t *watch_simulator_create_display(bool smoke)
{
#if defined(_WIN32)
    if (!smoke) {
        return lv_windows_create_display(L"MAGIC WATCH", WATCH_SIMULATOR_WIDTH,
                                         WATCH_SIMULATOR_HEIGHT, 200, false, true);
    }
#else
    (void)smoke;
#endif

    lv_display_t *display = lv_display_create(WATCH_SIMULATOR_WIDTH, WATCH_SIMULATOR_HEIGHT);
    if (display == NULL) {
        return NULL;
    }

    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, s_draw_buffer, NULL, sizeof(s_draw_buffer),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, watch_simulator_flush);
    return display;
}

static bool watch_simulator_has_label(lv_obj_t *screen, const char *text)
{
    int32_t index = 0;
    lv_obj_t *child;

    while ((child = lv_obj_get_child(screen, index)) != NULL) {
        if (lv_obj_check_type(child, &lv_label_class)
            && strcmp(lv_label_get_text(child), text) == 0) {
            return true;
        }
        ++index;
    }

    return false;
}

static bool watch_simulator_check_core(void)
{
    watch_core_t core;
    watch_event_t event = { .type = WATCH_EVENT_SELECT };
    watch_snapshot_t snapshot;

    if (!watch_core_init(&core) || !watch_core_dispatch_event(&core, &event)
        || !watch_core_read_snapshot(&core, &snapshot)) {
        return false;
    }

    return snapshot.page == WATCH_PAGE_LAUNCHER;
}

int main(int argc, char **argv)
{
    const bool smoke = (argc > 1) && (strcmp(argv[1], "--smoke") == 0);
    lv_display_t *display;
    lv_obj_t *screen;

    lv_init();
    display = watch_simulator_create_display(smoke);
    if (display == NULL) {
        fprintf(stderr, "watch_ui_smoke: display creation failed\n");
        return 1;
    }

    lv_display_set_default(display);
    lv_lock();
    f411_watch_ui_init(NULL);
    screen = screen_watchface_create();
    if (screen == NULL) {
        lv_unlock();
        fprintf(stderr, "watch_ui_smoke: screen creation failed\n");
        return 1;
    }
    lv_screen_load(screen);
    lv_unlock();
    (void)lv_timer_handler();

    if (!watch_simulator_has_label(screen, "MAGIC WATCH")
        || !watch_simulator_has_label(screen, "WATCHFACE")
        || !watch_simulator_has_label(screen, "M7 EDITOR UI")
        || !watch_simulator_check_core()) {
        fprintf(stderr, "watch_ui_smoke: FAIL\n");
        return 1;
    }

    printf("watch_ui_smoke: PASS display=240x280 ui=MAGIC WATCH core=LAUNCHER\n");
    fflush(stdout);

#if defined(_WIN32)
    if (!smoke) {
        for (;;) {
            uint32_t idle_ms = lv_timer_handler();
            if (idle_ms == LV_NO_TIMER_READY) {
                idle_ms = LV_DEF_REFR_PERIOD;
            }
            lv_sleep_ms(idle_ms);
        }
    }
#endif

    return 0;
}
