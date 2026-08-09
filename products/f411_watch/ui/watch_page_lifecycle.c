/**
 * @file watch_page_lifecycle.c
 * @brief Core-driven LVGL screen creation and destruction.
 */

#include "watch_page_lifecycle.h"

#include <stddef.h>

#include "screens/launcher/screen_launcher_gen.h"
#include "screens/settings/screen_settings_gen.h"
#include "screens/status/screen_status_gen.h"
#include "screens/watchface/screen_watchface_gen.h"

static lv_obj_t *watch_page_create(watch_page_t page)
{
    switch (page) {
    case WATCH_PAGE_WATCHFACE:
        return screen_watchface_create();
    case WATCH_PAGE_LAUNCHER:
        return screen_launcher_create();
    case WATCH_PAGE_STATUS:
        return screen_status_create();
    case WATCH_PAGE_SETTINGS:
        return screen_settings_create();
    case WATCH_PAGE_COUNT:
        return NULL;
    }

    return NULL;
}

static const char *watch_page_name(watch_page_t page)
{
    static const char *const names[WATCH_PAGE_COUNT] = {
        "WATCHFACE",
        "LAUNCHER",
        "STATUS",
        "SETTINGS",
    };

    if (page >= WATCH_PAGE_COUNT) {
        return "UNKNOWN";
    }

    return names[page];
}

static const char *watch_page_hint(const watch_snapshot_t *snapshot)
{
    if (snapshot->page == WATCH_PAGE_WATCHFACE) {
        return "SELECT: LAUNCHER";
    }

    if (snapshot->page == WATCH_PAGE_LAUNCHER) {
        return snapshot->launcher_index == 0U ? "SELECT: STATUS" : "SELECT: SETTINGS";
    }

    return "BACK: RETURN";
}

static bool watch_page_set_label(lv_obj_t *screen, int32_t index, const char *text)
{
    lv_obj_t *label = lv_obj_get_child(screen, index);

    if (label == NULL || !lv_obj_check_type(label, &lv_label_class)) {
        return false;
    }

    lv_label_set_text(label, text);
    return true;
}

static bool watch_page_render(lv_obj_t *screen, const watch_snapshot_t *snapshot)
{
    return watch_page_set_label(screen, 0, "MAGIC WATCH")
        && watch_page_set_label(screen, 1, watch_page_name(snapshot->page))
        && watch_page_set_label(screen, 2, watch_page_hint(snapshot));
}

bool watch_page_lifecycle_init(watch_page_lifecycle_t *lifecycle, lv_display_t *display)
{
    if (lifecycle == NULL || display == NULL) {
        return false;
    }

    *lifecycle = (watch_page_lifecycle_t) {
        .display = display,
        .active_page = WATCH_PAGE_COUNT,
    };
    lv_display_set_default(display);
    return true;
}

bool watch_page_lifecycle_apply(watch_page_lifecycle_t *lifecycle, const watch_snapshot_t *snapshot)
{
    lv_obj_t *next_screen;

    if (lifecycle == NULL || snapshot == NULL || lifecycle->display == NULL
        || snapshot->page >= WATCH_PAGE_COUNT) {
        return false;
    }

    if (lifecycle->active && lifecycle->active_page == snapshot->page
        && lifecycle->launcher_index == snapshot->launcher_index) {
        return true;
    }

    if (lifecycle->active && lifecycle->active_page == snapshot->page) {
        lifecycle->launcher_index = snapshot->launcher_index;
        return watch_page_render(lifecycle->active_screen, snapshot);
    }

    next_screen = watch_page_create(snapshot->page);
    if (next_screen == NULL || !watch_page_render(next_screen, snapshot)) {
        if (next_screen != NULL) {
            lv_obj_del(next_screen);
        }
        return false;
    }

    lv_display_set_default(lifecycle->display);
    lv_screen_load(next_screen);
    if (lifecycle->active_screen != NULL) {
        lv_obj_del(lifecycle->active_screen);
        lifecycle->destroyed_count++;
    }

    lifecycle->active_screen = next_screen;
    lifecycle->active_page = snapshot->page;
    lifecycle->launcher_index = snapshot->launcher_index;
    lifecycle->active = true;
    lifecycle->created_count++;
    return true;
}

void watch_page_lifecycle_deinit(watch_page_lifecycle_t *lifecycle)
{
    if (lifecycle == NULL) {
        return;
    }

    if (lifecycle->active_screen != NULL) {
        lv_obj_del(lifecycle->active_screen);
        lifecycle->active_screen = NULL;
        lifecycle->destroyed_count++;
    }

    lifecycle->active = false;
    lifecycle->active_page = WATCH_PAGE_COUNT;
}

lv_obj_t *watch_page_lifecycle_active_screen(const watch_page_lifecycle_t *lifecycle)
{
    if (lifecycle == NULL) {
        return NULL;
    }

    return lifecycle->active_screen;
}

void watch_page_lifecycle_read_stats(const watch_page_lifecycle_t *lifecycle,
                                     watch_page_lifecycle_stats_t *stats)
{
    if (lifecycle == NULL || stats == NULL) {
        return;
    }

    stats->created_count = lifecycle->created_count;
    stats->destroyed_count = lifecycle->destroyed_count;
}
