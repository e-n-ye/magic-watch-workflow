#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "f411_watch_ui.h"
#include "lvgl.h"
#include "watch_core.h"
#include "watch_input.h"
#include "watch_page_lifecycle.h"
#include "watch_ui_theme.h"

#if defined(_WIN32)
#include "drivers/windows/lv_windows_display.h"
#include "drivers/windows/lv_windows_input.h"
#endif

#define WATCH_SIMULATOR_WIDTH 240
#define WATCH_SIMULATOR_HEIGHT 280
#define WATCH_SIMULATOR_BUFFER_LINES 20
#define WATCH_SIMULATOR_LIFECYCLE_CYCLES 32U

static uint16_t s_draw_buffer[WATCH_SIMULATOR_WIDTH * WATCH_SIMULATOR_BUFFER_LINES];

#if defined(_WIN32)
static uint16_t watch_simulator_map_coord(int32_t value, int32_t extent, uint16_t limit)
{
    if (extent <= 0) {
        return 0U;
    }
    if (value < 0) {
        value = 0;
    } else if (value >= extent) {
        value = extent - 1;
    }
    return (uint16_t)((value * (int32_t)limit) / extent);
}

static watch_core_t s_interactive_core;
static bool s_key_q_down;
static bool s_key_e_down;
static bool s_key_enter_down;
static bool s_key_space_down;
static bool s_mouse_down;
static watch_input_t s_interactive_touch;
static watch_input_touch_t s_mouse_touch;
#endif

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

static bool watch_simulator_label_is(lv_obj_t *screen, const char *name, const char *text)
{
    lv_obj_t *child = lv_obj_find_by_name(screen, name);

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

static bool watch_simulator_theme_objects_are_ready(lv_obj_t *screen,
                                                    const watch_snapshot_t *snapshot)
{
    static const char *const launcher_names[] = {
        "launcher_item_status", "launcher_item_timer", "launcher_item_calendar",
        "launcher_item_settings",
    };
    static const char *const settings_names[] = {
        "settings_item_theme", "settings_item_brightness", "settings_item_time_format",
    };
    uint8_t index;

    if (screen == NULL || snapshot == NULL) {
        return false;
    }

    switch (snapshot->page) {
    case WATCH_PAGE_WATCHFACE:
        return lv_obj_find_by_name(screen, "watchface_date") != NULL
            && lv_obj_find_by_name(screen, "watchface_weekday") != NULL
            && lv_obj_find_by_name(screen, "watchface_battery") != NULL
            && lv_obj_find_by_name(screen, "watchface_steps") != NULL
            && lv_obj_find_by_name(screen, "watchface_status") != NULL;
    case WATCH_PAGE_LAUNCHER:
        if (lv_obj_find_by_name(screen, "launcher_list") == NULL) {
            return false;
        }
        for (index = 0U; index < WATCH_CORE_LAUNCHER_ITEM_COUNT; ++index) {
            if (lv_obj_find_by_name(screen, launcher_names[index]) == NULL) {
                return false;
            }
        }
        return true;
    case WATCH_PAGE_SETTINGS:
        if (lv_obj_find_by_name(screen, "settings_list") == NULL) {
            return false;
        }
        for (index = 0U; index < 3U; ++index) {
            if (lv_obj_find_by_name(screen, settings_names[index]) == NULL) {
                return false;
            }
        }
        return true;
    case WATCH_PAGE_STATUS:
        return lv_obj_find_by_name(screen, "status_summary") != NULL
            && lv_obj_find_by_name(screen, "status_sensors") != NULL
            && lv_obj_find_by_name(screen, "status_storage") != NULL;
    case WATCH_PAGE_TIMER:
        return lv_obj_find_by_name(screen, "timer_value") != NULL;
    case WATCH_PAGE_CALENDAR:
        return lv_obj_find_by_name(screen, "calendar_value") != NULL;
    case WATCH_PAGE_RESOURCES:
    case WATCH_PAGE_DIAGNOSTICS:
    case WATCH_PAGE_COUNT:
        return true;
    }

    return false;
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

    title_label = lv_obj_find_by_name(popup, "popup_title");
    message_label = lv_obj_find_by_name(popup, "popup_message");
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
        const bool background_ok = watch_simulator_background_is_opaque(screen);
        const bool brand_ok = watch_simulator_label_is(screen, "page_brand", "MAGIC WATCH");
        const bool title_ok = watch_simulator_label_is(screen, "page_title", page_name);
        const bool hint_ok = watch_simulator_label_is(screen, "page_hint", hint);
        const bool objects_ok = watch_simulator_theme_objects_are_ready(screen, snapshot);
        result = background_ok && brand_ok && title_ok && hint_ok && objects_ok;
    }
    lv_unlock();
    if (!result) {
        fprintf(stderr, "watch_ui_smoke: render failed page=%s hint=%s\n", page_name, hint);
    }
    return result;
}

static bool watch_simulator_dispatch_page(watch_core_t *core, watch_page_lifecycle_t *lifecycle,
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

static bool watch_simulator_dispatch_time(watch_core_t *core, watch_page_lifecycle_t *lifecycle,
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
        && watch_simulator_show_page(lifecycle, &snapshot, text, "ENCODER: LAUNCHER");
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

#if defined(_WIN32)
static bool watch_simulator_apply_input(watch_page_lifecycle_t *lifecycle,
                                        watch_event_type_t event_type)
{
    watch_event_t event = (watch_event_t) { .type = event_type };
    watch_command_t command;
    watch_snapshot_t snapshot;

    if (!watch_core_dispatch_event(&s_interactive_core, &event)) {
        return false;
    }
    while (watch_core_take_command(&s_interactive_core, &command)) {
        /* The simulator models the UI task consuming each core command. */
    }
    if (!watch_core_read_snapshot(&s_interactive_core, &snapshot)) {
        return false;
    }

    lv_lock();
    const bool applied = watch_page_lifecycle_apply(lifecycle, &snapshot);
    lv_unlock();
    return applied;
}

static void watch_simulator_poll_touch(watch_page_lifecycle_t *lifecycle, lv_indev_t *pointer_indev)
{
    HWND window;
    POINT point;
    RECT client;
    bool pressed;
    watch_event_t event;

    if (pointer_indev == NULL) {
        return;
    }
    window = lv_windows_get_indev_window_handle(pointer_indev);
    if (window == NULL || !GetCursorPos(&point) || !ScreenToClient(window, &point)
        || !GetClientRect(window, &client)) {
        return;
    }

    pressed = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    if (pressed && !s_mouse_down) {
        s_mouse_touch.start_x =
            watch_simulator_map_coord(point.x, client.right, WATCH_SIMULATOR_WIDTH);
        s_mouse_touch.start_y =
            watch_simulator_map_coord(point.y, client.bottom, WATCH_SIMULATOR_HEIGHT);
    } else if (!pressed && s_mouse_down) {
        s_mouse_touch.end_x =
            watch_simulator_map_coord(point.x, client.right, WATCH_SIMULATOR_WIDTH);
        s_mouse_touch.end_y =
            watch_simulator_map_coord(point.y, client.bottom, WATCH_SIMULATOR_HEIGHT);
        if (watch_input_submit_touch(&s_interactive_touch, &s_mouse_touch)
            && watch_input_take_event(&s_interactive_touch, &event)) {
            (void)watch_simulator_apply_input(lifecycle, event.type);
        }
    }
    s_mouse_down = pressed;
}

static void watch_simulator_poll_keys(watch_page_lifecycle_t *lifecycle)
{
    bool q_down = (GetAsyncKeyState('Q') & 0x8000) != 0;
    bool e_down = (GetAsyncKeyState('E') & 0x8000) != 0;
    bool enter_down = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
    bool space_down = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

    if (q_down && !s_key_q_down) {
        (void)watch_simulator_apply_input(lifecycle, WATCH_EVENT_UP);
    }
    if (e_down && !s_key_e_down) {
        (void)watch_simulator_apply_input(lifecycle, WATCH_EVENT_DOWN);
    }
    if (enter_down && !s_key_enter_down) {
        (void)watch_simulator_apply_input(lifecycle, WATCH_EVENT_ENCODER_PRESS);
    }
    if (space_down && !s_key_space_down) {
        (void)watch_simulator_apply_input(lifecycle, WATCH_EVENT_ENCODER_PRESS);
    }

    s_key_q_down = q_down;
    s_key_e_down = e_down;
    s_key_enter_down = enter_down;
    s_key_space_down = space_down;
}

static void watch_simulator_poll_input(watch_page_lifecycle_t *lifecycle, lv_indev_t *pointer_indev)
{
    watch_simulator_poll_keys(lifecycle);
    watch_simulator_poll_touch(lifecycle, pointer_indev);
}
#endif

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
    uint32_t cycle;

    if (!watch_core_init(&core)) {
        return false;
    }

    lv_lock();
    const bool initialized = watch_page_lifecycle_init(lifecycle, display);
    lv_unlock();
    if (!initialized || !watch_core_read_snapshot(&core, &snapshot)
        || !watch_simulator_show_page(lifecycle, &snapshot, "--:--:--", "ENCODER: LAUNCHER")
        || !watch_simulator_dispatch_time(&core, lifecycle, &time, "12:34:56")) {
        return false;
    }

    /* Touch SELECT is intentionally inert on the watchface; the crown owns entry. */
    if (!watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_SELECT, "12:34:56",
                                       "ENCODER: LAUNCHER")
        || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_ENCODER_PRESS, "LAUNCHER",
                                          "SELECT: STATUS")
        || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_SELECT, "STATUS",
                                          "BACK: RETURN")
        || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_ENCODER_PRESS, "12:34:56",
                                          "ENCODER: LAUNCHER")
        || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_ENCODER_PRESS, "LAUNCHER",
                                          "SELECT: STATUS")
        || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_DOWN, "LAUNCHER",
                                          "SELECT: TIMER")
        || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_SELECT, "TIMER",
                                          "BACK: RETURN")
        || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_ENCODER_PRESS, "12:34:56",
                                          "ENCODER: LAUNCHER")
        || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_ENCODER_PRESS, "LAUNCHER",
                                          "SELECT: TIMER")
        || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_DOWN, "LAUNCHER",
                                          "SELECT: CALENDAR")
        || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_SELECT, "CALENDAR",
                                          "BACK: RETURN")
        || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_ENCODER_PRESS, "12:34:56",
                                          "ENCODER: LAUNCHER")
        || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_ENCODER_PRESS, "LAUNCHER",
                                          "SELECT: CALENDAR")
        || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_DOWN, "LAUNCHER",
                                          "SELECT: SETTINGS")
        || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_SELECT, "SETTINGS",
                                          "BACK: RETURN")
        || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_ENCODER_PRESS, "12:34:56",
                                          "ENCODER: LAUNCHER")
        || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_BACK, "12:34:56",
                                          "ENCODER: LAUNCHER")) {
        return false;
    }

    for (cycle = 0U; cycle < WATCH_SIMULATOR_LIFECYCLE_CYCLES; ++cycle) {
        if (!watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_ENCODER_PRESS, "LAUNCHER",
                                           "SELECT: SETTINGS")
            || !watch_simulator_dispatch_page(&core, lifecycle, WATCH_EVENT_ENCODER_PRESS,
                                              "12:34:56", "ENCODER: LAUNCHER")) {
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
    if (!watch_simulator_show_page(lifecycle, &diagnostics_snapshot, "DIAGNOSTICS", "BACK: RETURN")
        || watch_page_lifecycle_active_popup(lifecycle) != NULL) {
        return false;
    }

    diagnostics_snapshot.popup_visible = true;
    if (!watch_simulator_show_page(lifecycle, &diagnostics_snapshot, "DIAGNOSTICS", "BACK: RETURN")
        || !watch_simulator_popup_is(lifecycle, "DIAGNOSTICS", "CORE READY")
        || !watch_simulator_show_page(lifecycle, &diagnostics_snapshot, "DIAGNOSTICS",
                                      "BACK: RETURN")
        || !watch_simulator_popup_is(lifecycle, "DIAGNOSTICS", "CORE READY")) {
        return false;
    }

    diagnostics_snapshot.popup_visible = false;
    if (!watch_simulator_show_page(lifecycle, &diagnostics_snapshot, "DIAGNOSTICS", "BACK: RETURN")
        || watch_page_lifecycle_active_popup(lifecycle) != NULL
        || !watch_simulator_show_page(lifecycle, &snapshot, "12:34:56", "ENCODER: LAUNCHER")) {
        return false;
    }

    watch_page_lifecycle_read_stats(lifecycle, &stats);
    return stats.created_count > 0U && stats.destroyed_count == (stats.created_count - 1U)
        && stats.popup_created_count == 3U && stats.popup_destroyed_count == 3U;
}

int main(int argc, char **argv)
{
    const bool smoke = (argc > 1) && (strcmp(argv[1], "--smoke") == 0);
    lv_display_t *display;
    watch_page_lifecycle_t lifecycle;
    watch_page_lifecycle_stats_t stats;
#if defined(_WIN32)
    lv_indev_t *pointer_indev = NULL;
#endif

    lv_init();
    display = watch_simulator_create_display(smoke);
    if (display == NULL) {
        fprintf(stderr, "watch_ui_smoke: display creation failed\n");
        return 1;
    }

#if defined(_WIN32)
    if (!smoke) {
        pointer_indev = lv_windows_acquire_pointer_indev(display);
        (void)lv_windows_acquire_keypad_indev(display);
        (void)lv_windows_acquire_encoder_indev(display);
    }
#endif

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
        watch_snapshot_t interactive_snapshot;

        if (!watch_core_init(&s_interactive_core)
            || !watch_core_read_snapshot(&s_interactive_core, &interactive_snapshot)
            || !watch_simulator_show_page(&lifecycle, &interactive_snapshot, "--:--:--",
                                          "ENCODER: LAUNCHER")
            || !watch_input_init(&s_interactive_touch)) {
            fprintf(stderr, "watch_ui_simulator: interactive input initialization failed\n");
            return 1;
        }
        /* The native Windows backend allocates its framebuffer from a timer.
         * Let that
         * timer run before forcing the first visible frame. */
        lv_sleep_ms(LV_DEF_REFR_PERIOD + 1U);
        (void)lv_timer_handler();
        lv_lock();
        lv_obj_invalidate(watch_page_lifecycle_active_screen(&lifecycle));
        lv_refr_now(display);
        lv_unlock();
    }
#endif

    watch_page_lifecycle_read_stats(&lifecycle, &stats);
    printf("watch_ui_smoke: PASS display=240x280 pages=%u lifecycle_cycles=%u creates=%lu "
           "destroys=%lu popups=%lu/%lu active=WATCHFACE\n",
           WATCH_PAGE_COUNT, WATCH_SIMULATOR_LIFECYCLE_CYCLES, (unsigned long)stats.created_count,
           (unsigned long)stats.destroyed_count, (unsigned long)stats.popup_created_count,
           (unsigned long)stats.popup_destroyed_count);
    fflush(stdout);

#if defined(_WIN32)
    if (!smoke) {
        for (;;) {
            watch_simulator_poll_input(&lifecycle, pointer_indev);
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
