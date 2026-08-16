#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "f411_watch_ui.h"
#include "lvgl.h"
#include "watch_core.h"
#include "watch_page_lifecycle.h"

#if defined(_WIN32)
#include "drivers/windows/lv_windows_display.h"
#endif

#define WATCH_SIMULATOR_WIDTH 240
#define WATCH_SIMULATOR_HEIGHT 280
#define WATCH_SIMULATOR_BUFFER_LINES 20
#define WATCH_SIMULATOR_LIFECYCLE_CYCLES 32U
#define WATCH_SIMULATOR_EXPECTED_INITIAL_PAGES 9U
#define WATCH_SIMULATOR_EXPECTED_CYCLE_PAGES 8U

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

static bool watch_simulator_label_is(lv_obj_t *screen, int32_t index, const char *text)
{
    lv_obj_t *child = lv_obj_get_child(screen, index);

    if (child == NULL || !lv_obj_check_type(child, &lv_label_class)) {
        return false;
    }

    return strcmp(lv_label_get_text(child), text) == 0;
}

static bool watch_simulator_background_is_opaque(lv_obj_t *screen)
{
    return lv_color_eq(lv_obj_get_style_bg_color(screen, LV_PART_MAIN), lv_color_hex(0x101820))
        && lv_obj_get_style_bg_opa(screen, LV_PART_MAIN) == LV_OPA_COVER;
}

static bool watch_simulator_popup_is(watch_page_lifecycle_t *lifecycle, const char *title,
                                     const char *message)
{
    lv_obj_t *popup = watch_page_lifecycle_active_popup(lifecycle);
    lv_obj_t *title_label;
    lv_obj_t *message_label;

    if (popup == NULL) {
        return false;
    }

    title_label = lv_obj_get_child(popup, 0);
    message_label = lv_obj_get_child(popup, 1);
    return title_label != NULL && message_label != NULL
        && lv_obj_check_type(title_label, &lv_label_class)
        && lv_obj_check_type(message_label, &lv_label_class)
        && strcmp(lv_label_get_text(title_label), title) == 0
        && strcmp(lv_label_get_text(message_label), message) == 0;
}

static bool watch_simulator_show_page(watch_page_lifecycle_t *lifecycle,
                                      const watch_snapshot_t *snapshot, const char *page_name,
                                      const char *hint)
{
    lv_obj_t *screen;
    bool result;

    lv_lock();
    result = watch_page_lifecycle_apply(lifecycle, snapshot);
    screen = watch_page_lifecycle_active_screen(lifecycle);
    if (result && screen != NULL) {
        result = watch_simulator_background_is_opaque(screen)
            && watch_simulator_label_is(screen, 0, "MAGIC WATCH")
            && watch_simulator_label_is(screen, 1, page_name)
            && watch_simulator_label_is(screen, 2, hint);
    }
    lv_unlock();
    return result;
}

static bool watch_simulator_dispatch_page(watch_core_t *core,
                                           watch_page_lifecycle_t *lifecycle,
                                           watch_event_type_t event_type, const char *page_name,
                                           const char *hint)
{
    watch_event_t event = (watch_event_t) { .type = event_type };
    watch_command_t command;
    watch_snapshot_t snapshot;

    if (!watch_core_dispatch_event(core, &event)) {
        return false;
    }

    while (watch_core_take_command(core, &command)) {
        /* The simulator models the UI task consuming each core command. */
    }

    if (!watch_core_read_snapshot(core, &snapshot)) {
        return false;
    }

    return watch_simulator_show_page(lifecycle, &snapshot, page_name, hint);
}

static bool watch_simulator_dispatch_time(watch_core_t *core,
                                          watch_page_lifecycle_t *lifecycle,
                                          const watch_time_value_t *time, const char *text)
{
    watch_event_t event = {
        .type = WATCH_EVENT_TIME_UPDATED,
        .time = *time,
    };
    watch_command_t command;
    watch_snapshot_t snapshot;

    if (!watch_core_dispatch_event(core, &event)) {
        return false;
    }

    while (watch_core_take_command(core, &command)) {
        /* The simulator models the UI task consuming each core command. */
    }

    return watch_core_read_snapshot(core, &snapshot)
        && watch_simulator_show_page(lifecycle, &snapshot, text, "SELECT: LAUNCHER");
}

static bool watch_simulator_show_popup(watch_page_lifecycle_t *lifecycle, const char *title,
                                       const char *message)
{
    bool result;

    lv_lock();
    result = watch_page_lifecycle_show_popup(lifecycle, title, message)
        && watch_simulator_popup_is(lifecycle, title, message);
    lv_unlock();
    return result;
}

static bool watch_simulator_check_lifecycle(lv_display_t *display,
                                            watch_page_lifecycle_t *lifecycle)
{
    watch_core_t core;
    watch_snapshot_t snapshot;
    watch_snapshot_t diagnostics_snapshot;
    watch_page_lifecycle_stats_t stats;
    const watch_time_value_t time = {
        .year = 2026U,
        .month = 8U,
        .day = 16U,
        .weekday = WATCH_TIME_WEEKDAY_SUNDAY,
        .hour = 12U,
        .minute = 34U,
        .second = 56U,
    };
    uint32_t expected_pages;
    uint32_t cycle;

    if (!watch_core_init(&core)) {
        return false;
    }

    lv_lock();
    const bool initialized = watch_page_lifecycle_init(lifecycle, display);
    lv_unlock();
    if (!initialized || !watch_core_read_snapshot(&core, &snapshot)
        || !watch_simulator_show_page(lifecycle, &snapshot, "--:--:--", "SELECT: LAUNCHER")
        || !watch_simulator_dispatch_time(&core, lifecycle, &time, "12:34:56")) {
        return false;
    }

    if (!watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_SELECT, "LAUNCHER",
                                       "SELECT: STATUS")
        || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_SELECT, "STATUS",
                                          "BACK: RETURN")
        || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_BACK, "LAUNCHER",
                                          "SELECT: STATUS")
        || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_DOWN, "LAUNCHER",
                                          "SELECT: SETTINGS")
        || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_SELECT, "SETTINGS",
                                          "BACK: RETURN")
        || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_BACK, "LAUNCHER",
                                          "SELECT: SETTINGS")
        || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_DOWN, "LAUNCHER",
                                          "SELECT: RESOURCES")
        || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_SELECT, "RESOURCES",
                                          "BACK: RETURN")
        || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_BACK, "LAUNCHER",
                                           "SELECT: RESOURCES")
        || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_BACK, "12:34:56",
                                           "SELECT: LAUNCHER")) {
        return false;
    }

    for (cycle = 0U; cycle < WATCH_SIMULATOR_LIFECYCLE_CYCLES; ++cycle) {
        if (!watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_SELECT, "LAUNCHER",
                                           "SELECT: RESOURCES")
            || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_UP, "LAUNCHER",
                                              "SELECT: SETTINGS")
            || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_UP, "LAUNCHER",
                                              "SELECT: STATUS")
            || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_SELECT, "STATUS",
                                              "BACK: RETURN")
            || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_BACK, "LAUNCHER",
                                              "SELECT: STATUS")
            || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_DOWN, "LAUNCHER",
                                              "SELECT: SETTINGS")
            || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_SELECT, "SETTINGS",
                                              "BACK: RETURN")
            || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_BACK, "LAUNCHER",
                                              "SELECT: SETTINGS")
            || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_DOWN, "LAUNCHER",
                                              "SELECT: RESOURCES")
            || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_SELECT, "RESOURCES",
                                              "BACK: RETURN")
            || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_BACK, "LAUNCHER",
                                               "SELECT: RESOURCES")
            || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_BACK, "12:34:56",
                                               "SELECT: LAUNCHER")) {
            return false;
        }
    }

    if (!watch_core_read_snapshot(&core, &snapshot)) {
        return false;
    }

    if (!watch_simulator_show_popup(lifecycle, "CORE", "READY")
        || !watch_simulator_show_popup(lifecycle, "CORE", "UPDATED")) {
        return false;
    }

    diagnostics_snapshot = snapshot;
    diagnostics_snapshot.page = WATCH_PAGE_DIAGNOSTICS;
    diagnostics_snapshot.page_depth = 1U;
    diagnostics_snapshot.popup_visible = false;
    if (!watch_simulator_show_page(lifecycle, &diagnostics_snapshot, "DIAGNOSTICS",
                                   "BACK: RETURN")
        || watch_page_lifecycle_active_popup(lifecycle) != NULL) {
        return false;
    }

    diagnostics_snapshot.popup_visible = true;
    if (!watch_simulator_show_page(lifecycle, &diagnostics_snapshot, "DIAGNOSTICS",
                                   "BACK: RETURN")
        || !watch_simulator_popup_is(lifecycle, "DIAGNOSTICS", "CORE READY")
        || !watch_simulator_show_page(lifecycle, &diagnostics_snapshot, "DIAGNOSTICS",
                                      "BACK: RETURN")
        || !watch_simulator_popup_is(lifecycle, "DIAGNOSTICS", "CORE READY")) {
        return false;
    }

    diagnostics_snapshot.popup_visible = false;
    if (!watch_simulator_show_page(lifecycle, &diagnostics_snapshot, "DIAGNOSTICS",
                                   "BACK: RETURN")
        || watch_page_lifecycle_active_popup(lifecycle) != NULL
        || !watch_simulator_show_page(lifecycle, &snapshot, "12:34:56", "SELECT: LAUNCHER")) {
        return false;
    }

    watch_page_lifecycle_read_stats(lifecycle, &stats);
    expected_pages = WATCH_SIMULATOR_EXPECTED_INITIAL_PAGES
        + (WATCH_SIMULATOR_LIFECYCLE_CYCLES * WATCH_SIMULATOR_EXPECTED_CYCLE_PAGES) + 2U;
    return stats.created_count == expected_pages
        && stats.destroyed_count == (stats.created_count - 1U) && stats.popup_created_count == 3U
        && stats.popup_destroyed_count == 3U;
}

int main(int argc, char **argv)
{
    const bool smoke = (argc > 1) && (strcmp(argv[1], "--smoke") == 0);
    lv_display_t *display;
    watch_page_lifecycle_t lifecycle;
    watch_page_lifecycle_stats_t stats;

    lv_init();
    display = watch_simulator_create_display(smoke);
    if (display == NULL) {
        fprintf(stderr, "watch_ui_smoke: display creation failed\n");
        return 1;
    }

    lv_display_set_default(display);
    lv_lock();
    f411_watch_ui_init(NULL);
    lv_unlock();

    if (!watch_simulator_check_lifecycle(display, &lifecycle)) {
        fprintf(stderr, "watch_ui_smoke: lifecycle failed\n");
        return 1;
    }

    (void)lv_timer_handler();

#if defined(_WIN32)
    if (!smoke) {
        /* The native Windows backend allocates its framebuffer from a timer.
         * Let that timer run before forcing the first visible frame. */
        lv_sleep_ms(LV_DEF_REFR_PERIOD + 1U);
        (void)lv_timer_handler();
        lv_lock();
        lv_obj_invalidate(watch_page_lifecycle_active_screen(&lifecycle));
        lv_refr_now(display);
        lv_unlock();
    }
#endif

    watch_page_lifecycle_read_stats(&lifecycle, &stats);
    printf(
        "watch_ui_smoke: PASS display=240x280 pages=6 lifecycle_cycles=%u creates=%lu "
        "destroys=%lu popups=%lu/%lu active=WATCHFACE\n",
        WATCH_SIMULATOR_LIFECYCLE_CYCLES, (unsigned long)stats.created_count,
        (unsigned long)stats.destroyed_count, (unsigned long)stats.popup_created_count,
        (unsigned long)stats.popup_destroyed_count);
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

    lv_lock();
    watch_page_lifecycle_deinit(&lifecycle);
    lv_unlock();
    watch_page_lifecycle_read_stats(&lifecycle, &stats);
    if (stats.created_count != stats.destroyed_count) {
        fprintf(stderr, "watch_ui_smoke: lifecycle leak\n");
        return 1;
    }

    return 0;
}
