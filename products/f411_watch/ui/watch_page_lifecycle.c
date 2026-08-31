/**
 * @file watch_page_lifecycle.c
 * @brief Core-driven LVGL screen creation and destruction.
 */

#include "watch_page_lifecycle.h"

#include <stddef.h>
#include <string.h>

#include "screens/launcher/screen_launcher_gen.h"
#include "screens/diagnostics/screen_diagnostics_gen.h"
#include "screens/resources/screen_resources_gen.h"
#include "screens/settings/screen_settings_gen.h"
#include "screens/status/screen_status_gen.h"
#include "screens/watchface/screen_watchface_gen.h"
#include "watch_ui_theme.h"

static lv_obj_t *watch_page_create(watch_page_t page)
{
    lv_obj_t *screen;

    switch (page) {
    case WATCH_PAGE_WATCHFACE:
        return screen_watchface_create();
    case WATCH_PAGE_LAUNCHER:
        return screen_launcher_create();
    case WATCH_PAGE_STATUS:
        return screen_status_create();
    case WATCH_PAGE_TIMER:
    case WATCH_PAGE_CALENDAR:
        /* P1 keeps navigation testable before the P2/P4 XML pages are exported. */
        screen = lv_obj_create(NULL);
        if (screen == NULL) {
            return NULL;
        }
        lv_obj_set_name_static(screen,
                               page == WATCH_PAGE_TIMER ? "screen_timer_#" : "screen_calendar_#");
        lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x101820), LV_PART_MAIN);
        {
            lv_obj_t *brand = lv_label_create(screen);
            lv_obj_t *title = lv_label_create(screen);
            lv_obj_t *hint = lv_label_create(screen);
            lv_obj_t *detail = lv_label_create(screen);
            if (brand == NULL || title == NULL || hint == NULL || detail == NULL) {
                lv_obj_del(screen);
                return NULL;
            }
            lv_obj_set_name_static(brand, "page_brand");
            lv_obj_set_name_static(title, "page_title");
            lv_obj_set_name_static(hint, "page_hint");
            lv_obj_set_name_static(detail, page == WATCH_PAGE_TIMER ? "timer_value"
                                                                      : "calendar_value");
            lv_label_set_text(brand, "MAGIC WATCH");
            lv_label_set_text(title, page == WATCH_PAGE_TIMER ? "TIMER" : "CALENDAR");
            lv_label_set_text(hint, "BACK: RETURN");
            lv_label_set_text(detail, page == WATCH_PAGE_TIMER ? "00:00.0" : "CALENDAR READY");
            lv_obj_set_align(brand, LV_ALIGN_TOP_MID);
            lv_obj_set_y(brand, 22);
            lv_obj_set_align(title, LV_ALIGN_CENTER);
            lv_obj_set_y(title, -8);
            lv_obj_set_align(detail, LV_ALIGN_CENTER);
            lv_obj_set_y(detail, 32);
            lv_obj_set_align(hint, LV_ALIGN_BOTTOM_MID);
            lv_obj_set_y(hint, -22);
            lv_obj_set_style_text_color(brand, lv_color_hex(0xF4F7FA), LV_PART_MAIN);
            lv_obj_set_style_text_color(title, lv_color_hex(0x64D2FF), LV_PART_MAIN);
            lv_obj_set_style_text_color(hint, lv_color_hex(0xB8C7D9), LV_PART_MAIN);
            lv_obj_set_style_text_color(detail, lv_color_hex(0x64D2FF), LV_PART_MAIN);
        }
        return screen;
    case WATCH_PAGE_SETTINGS:
        return screen_settings_create();
    case WATCH_PAGE_RESOURCES:
        return screen_resources_create();
    case WATCH_PAGE_DIAGNOSTICS:
        return screen_diagnostics_create();
    case WATCH_PAGE_COUNT:
        return NULL;
    }

    return NULL;
}

static const char *watch_page_name(watch_page_t page)
{
    static const char *const names[WATCH_PAGE_COUNT] = {
        "WATCHFACE", "LAUNCHER", "STATUS",    "TIMER",
        "CALENDAR",  "SETTINGS", "RESOURCES", "DIAGNOSTICS",
    };

    if (page >= WATCH_PAGE_COUNT) {
        return "UNKNOWN";
    }

    return names[page];
}

static const char *watch_page_hint(const watch_snapshot_t *snapshot)
{
    if (snapshot->page == WATCH_PAGE_WATCHFACE) {
        return "ENCODER: LAUNCHER";
    }

    if (snapshot->page == WATCH_PAGE_LAUNCHER) {
        const watch_app_entry_t *app = watch_core_get_launcher_app(snapshot->launcher_index);
        if (app != NULL) {
            static const char *const hints[WATCH_APP_COUNT] = {
                "SELECT: STATUS",   "SELECT: TIMER",     "SELECT: CALENDAR",
                "SELECT: SETTINGS", "SELECT: RESOURCES", "SELECT: DIAGNOSTICS",
            };
            return app->id < WATCH_APP_COUNT ? hints[app->id] : "SELECT: APP";
        }
        return "SELECT: APP";
    }

    return "BACK: RETURN";
}

static void watch_page_bind_labels(lv_obj_t *screen)
{
    uint32_t count = lv_obj_get_child_count(screen);
    uint32_t index;

    for (index = 0U; index < count; ++index) {
        lv_obj_t *label = lv_obj_get_child(screen, (int32_t)index);
        const char *text;

        if (label == NULL || !lv_obj_check_type(label, &lv_label_class)
            || lv_obj_get_name(label) != NULL) {
            continue;
        }

        text = lv_label_get_text(label);
        if (strcmp(text, "MAGIC WATCH") == 0) {
            lv_obj_set_name_static(label, "page_brand");
        } else if (strcmp(text, "BACK: RETURN") == 0 || strncmp(text, "SELECT:", 7U) == 0) {
            lv_obj_set_name_static(label, "page_hint");
        } else {
            lv_obj_set_name_static(label, "page_title");
        }
    }
}

static bool watch_page_set_label(lv_obj_t *screen, const char *name, const char *text)
{
    lv_obj_t *label = lv_obj_find_by_name(screen, name);

    if (label == NULL || !lv_obj_check_type(label, &lv_label_class)) {
        return false;
    }

    lv_label_set_text(label, text);
    return true;
}

static bool watch_page_render(lv_obj_t *screen, const watch_snapshot_t *snapshot)
{
    char watchface_time[WATCH_TIME_HMS_TEXT_SIZE];
    const char *title = watch_page_name(snapshot->page);

    if (snapshot->page == WATCH_PAGE_WATCHFACE) {
        title = "--:--:--";
        if (snapshot->time_valid
            && watch_time_format_hms(&snapshot->time, watchface_time, sizeof(watchface_time))) {
            title = watchface_time;
        }
    }

    return watch_page_set_label(screen, "page_brand", "MAGIC WATCH")
        && watch_page_set_label(screen, "page_title", title)
        && watch_page_set_label(screen, "page_hint", watch_page_hint(snapshot));
}

static bool watch_page_time_changed(const watch_page_lifecycle_t *lifecycle,
                                    const watch_snapshot_t *snapshot)
{
    return lifecycle->time_valid != snapshot->time_valid
        || (snapshot->time_valid && !watch_time_equal(&lifecycle->time, &snapshot->time));
}

static bool watch_page_sync_popup(watch_page_lifecycle_t *lifecycle,
                                  const watch_snapshot_t *snapshot)
{
    if (snapshot->page != WATCH_PAGE_DIAGNOSTICS || !snapshot->popup_visible) {
        watch_page_lifecycle_close_popup(lifecycle);
        return true;
    }

    if (lifecycle->active_popup != NULL) {
        return true;
    }

    return watch_page_lifecycle_show_popup(lifecycle, "DIAGNOSTICS", "CORE READY");
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

bool watch_page_lifecycle_show_popup(watch_page_lifecycle_t *lifecycle, const char *title,
                                     const char *message)
{
    lv_obj_t *popup;
    lv_obj_t *title_label;
    lv_obj_t *message_label;
    const watch_ui_palette_t *palette;

    if (lifecycle == NULL || lifecycle->active_screen == NULL || title == NULL || message == NULL) {
        return false;
    }

    palette = watch_ui_theme_palette(watch_ui_theme_get_mode());
    watch_page_lifecycle_close_popup(lifecycle);
    popup = lv_obj_create(lifecycle->active_screen);
    if (popup == NULL) {
        return false;
    }

    lv_obj_set_size(popup, 208, 104);
    lv_obj_center(popup);
    lv_obj_set_style_bg_opa(popup, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(popup, palette->surface, LV_PART_MAIN);
    lv_obj_set_style_border_width(popup, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(popup, palette->accent, LV_PART_MAIN);

    title_label = lv_label_create(popup);
    message_label = lv_label_create(popup);
    if (title_label == NULL || message_label == NULL) {
        lv_obj_del(popup);
        return false;
    }

    lv_obj_set_width(title_label, 184);
    lv_obj_set_width(message_label, 184);
    lv_obj_set_name_static(popup, "page_popup");
    lv_obj_set_name_static(title_label, "popup_title");
    lv_obj_set_name_static(message_label, "popup_message");
    lv_label_set_text(title_label, title);
    lv_label_set_text(message_label, message);
    lv_obj_set_align(title_label, LV_ALIGN_TOP_MID);
    lv_obj_set_y(title_label, 8);
    lv_obj_set_align(message_label, LV_ALIGN_TOP_MID);
    lv_obj_set_y(message_label, 36);
    lv_obj_set_style_text_color(title_label, palette->text, LV_PART_MAIN);
    lv_obj_set_style_text_color(message_label, palette->muted, LV_PART_MAIN);

    lifecycle->active_popup = popup;
    lifecycle->popup_created_count++;
    return true;
}

void watch_page_lifecycle_close_popup(watch_page_lifecycle_t *lifecycle)
{
    if (lifecycle == NULL || lifecycle->active_popup == NULL) {
        return;
    }

    lv_obj_del(lifecycle->active_popup);
    lifecycle->active_popup = NULL;
    lifecycle->popup_destroyed_count++;
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
        if (snapshot->page == WATCH_PAGE_WATCHFACE
            && watch_page_time_changed(lifecycle, snapshot)) {
            lifecycle->time_valid = snapshot->time_valid;
            lifecycle->time = snapshot->time;
            return watch_page_render(lifecycle->active_screen, snapshot)
                && watch_ui_theme_apply(lifecycle->active_screen, snapshot)
                && watch_page_sync_popup(lifecycle, snapshot);
        }

        lifecycle->time_valid = snapshot->time_valid;
        lifecycle->time = snapshot->time;
        return watch_ui_theme_apply(lifecycle->active_screen, snapshot)
            && watch_page_sync_popup(lifecycle, snapshot);
    }

    if (lifecycle->active && lifecycle->active_page == snapshot->page) {
        lifecycle->launcher_index = snapshot->launcher_index;
        lifecycle->time_valid = snapshot->time_valid;
        lifecycle->time = snapshot->time;
        return watch_page_render(lifecycle->active_screen, snapshot)
            && watch_ui_theme_apply(lifecycle->active_screen, snapshot)
            && watch_page_sync_popup(lifecycle, snapshot);
    }

    next_screen = watch_page_create(snapshot->page);
    if (next_screen != NULL) {
        watch_page_bind_labels(next_screen);
    }
    if (next_screen == NULL || !watch_page_render(next_screen, snapshot)
        || !watch_ui_theme_apply(next_screen, snapshot)) {
        if (next_screen != NULL) {
            lv_obj_del(next_screen);
        }
        return false;
    }

    watch_page_lifecycle_close_popup(lifecycle);
    lv_display_set_default(lifecycle->display);
    lv_screen_load(next_screen);
    if (lifecycle->active_screen != NULL) {
        lv_obj_del(lifecycle->active_screen);
        lifecycle->destroyed_count++;
    }

    lifecycle->active_screen = next_screen;
    lifecycle->active_page = snapshot->page;
    lifecycle->launcher_index = snapshot->launcher_index;
    lifecycle->time_valid = snapshot->time_valid;
    lifecycle->time = snapshot->time;
    lifecycle->active = true;
    lifecycle->created_count++;
    return watch_page_sync_popup(lifecycle, snapshot);
}

void watch_page_lifecycle_deinit(watch_page_lifecycle_t *lifecycle)
{
    if (lifecycle == NULL) {
        return;
    }

    watch_page_lifecycle_close_popup(lifecycle);
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

lv_obj_t *watch_page_lifecycle_active_popup(const watch_page_lifecycle_t *lifecycle)
{
    if (lifecycle == NULL) {
        return NULL;
    }

    return lifecycle->active_popup;
}

void watch_page_lifecycle_read_stats(const watch_page_lifecycle_t *lifecycle,
                                     watch_page_lifecycle_stats_t *stats)
{
    if (lifecycle == NULL || stats == NULL) {
        return;
    }

    stats->created_count = lifecycle->created_count;
    stats->destroyed_count = lifecycle->destroyed_count;
    stats->popup_created_count = lifecycle->popup_created_count;
    stats->popup_destroyed_count = lifecycle->popup_destroyed_count;
}
