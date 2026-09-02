/**
 * @file watch_ui_theme.c
 * @brief Compact user-owned visual system for the 240x280 F411 UI.
 */

#include "watch_ui_theme.h"

#include <stdio.h>
#include <string.h>

#define WATCH_UI_SCREEN_WIDTH 240
#define WATCH_UI_MARGIN 16
#define WATCH_UI_CONTENT_WIDTH (WATCH_UI_SCREEN_WIDTH - (WATCH_UI_MARGIN * 2))
#define WATCH_UI_ROW_HEIGHT 40
#define WATCH_UI_ROW_GAP 4

static const watch_ui_palette_t s_dark_palette = {
    .background = LV_COLOR_MAKE(0x10, 0x18, 0x20),
    .surface = LV_COLOR_MAKE(0x1B, 0x29, 0x35),
    .surface_alt = LV_COLOR_MAKE(0x24, 0x37, 0x46),
    .accent = LV_COLOR_MAKE(0x64, 0xD2, 0xFF),
    .text = LV_COLOR_MAKE(0xF4, 0xF7, 0xFA),
    .muted = LV_COLOR_MAKE(0xB8, 0xC7, 0xD9),
    .border = LV_COLOR_MAKE(0x38, 0x51, 0x63),
    .selected = LV_COLOR_MAKE(0x24, 0x54, 0x6B),
    .success = LV_COLOR_MAKE(0x79, 0xE2, 0xA3),
    .degraded = LV_COLOR_MAKE(0xFF, 0xB4, 0x54),
};

static const watch_ui_palette_t s_light_palette = {
    .background = LV_COLOR_MAKE(0xF4, 0xF7, 0xFA),
    .surface = LV_COLOR_MAKE(0xFF, 0xFF, 0xFF),
    .surface_alt = LV_COLOR_MAKE(0xE8, 0xEE, 0xF3),
    .accent = LV_COLOR_MAKE(0x08, 0x7E, 0xA4),
    .text = LV_COLOR_MAKE(0x10, 0x18, 0x20),
    .muted = LV_COLOR_MAKE(0x4A, 0x5A, 0x6A),
    .border = LV_COLOR_MAKE(0xC5, 0xD0, 0xDB),
    .selected = LV_COLOR_MAKE(0xBC, 0xE7, 0xF5),
    .success = LV_COLOR_MAKE(0x24, 0x7A, 0x4B),
    .degraded = LV_COLOR_MAKE(0xA4, 0x5A, 0x00),
};

static watch_ui_theme_mode_t s_theme_mode;
static lv_obj_t *watch_ui_theme_find_label(lv_obj_t *parent, const char *name)
{
    lv_obj_t *label = lv_obj_find_by_name(parent, name);

    if (label == NULL || !lv_obj_check_type(label, &lv_label_class)) {
        return NULL;
    }

    return label;
}

static lv_obj_t *watch_ui_theme_find_object(lv_obj_t *parent, const char *name)
{
    lv_obj_t *object = lv_obj_find_by_name(parent, name);
    return object;
}

static void watch_ui_theme_set_text(lv_obj_t *label, const char *text,
                                    const watch_ui_palette_t *palette, lv_color_t color)
{
    (void)palette;
    lv_obj_set_style_text_color(label, color, LV_PART_MAIN);
    lv_label_set_text(label, text);
}

static void watch_ui_theme_style_label(lv_obj_t *label, const watch_ui_palette_t *palette,
                                       lv_color_t color, lv_coord_t width, lv_coord_t x,
                                       lv_coord_t y)
{
    /* XML owns the fixed layout; keep this hook limited to dynamic content. */
    (void)label;
    (void)palette;
    (void)color;
    (void)width;
    (void)x;
    (void)y;
}

static void watch_ui_theme_style_card(lv_obj_t *card, const watch_ui_palette_t *palette,
                                      bool selected)
{
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(card, selected ? palette->selected : palette->surface, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, selected ? 2 : 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, selected ? palette->accent : palette->border, LV_PART_MAIN);
    if (selected) {
        lv_obj_add_state(card, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(card, LV_STATE_CHECKED);
    }
}

static bool watch_ui_theme_is_surface(const char *name)
{
    static const char *const names[] = {
        "watchface_summary",       "launcher_item_status", "launcher_item_timer",
        "launcher_item_calendar",  "launcher_item_settings", "settings_item_theme",
        "settings_item_brightness", "settings_item_time_format",
    };
    size_t index;

    if (name == NULL) {
        return false;
    }
    for (index = 0U; index < sizeof(names) / sizeof(names[0]); ++index) {
        if (strcmp(name, names[index]) == 0) {
            return true;
        }
    }
    return false;
}

static void watch_ui_theme_style_tree(lv_obj_t *object, const watch_ui_palette_t *palette)
{
    const char *name = lv_obj_get_name(object);
    uint32_t index;

    if (name != NULL && strncmp(name, "screen_", 7U) == 0) {
        lv_obj_set_style_bg_opa(object, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(object, palette->background, LV_PART_MAIN);
        lv_obj_set_style_text_color(object, palette->text, LV_PART_MAIN);
    } else if (watch_ui_theme_is_surface(name)) {
        lv_obj_set_style_bg_opa(object, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(object, palette->surface, LV_PART_MAIN);
        lv_obj_set_style_border_width(object, 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(object, palette->border, LV_PART_MAIN);
    }

    if (lv_obj_check_type(object, &lv_label_class)) {
        lv_color_t color = palette->text;
        if (name != NULL && (strcmp(name, "page_title") == 0 || strcmp(name, "status_summary") == 0)) {
            color = palette->accent;
        } else if (name != NULL && (strcmp(name, "page_brand") == 0 || strcmp(name, "page_hint") == 0)) {
            color = palette->muted;
        }
        lv_obj_set_style_text_color(object, color, LV_PART_MAIN);
    }

    for (index = 0U; index < lv_obj_get_child_count(object); ++index) {
        lv_obj_t *child = lv_obj_get_child(object, (int32_t)index);
        if (child != NULL) {
            watch_ui_theme_style_tree(child, palette);
        }
    }
}

static void watch_ui_theme_style_frame(lv_obj_t *screen, const watch_ui_palette_t *palette)
{
    watch_ui_theme_style_tree(screen, palette);
}

static const char *watch_ui_theme_weekday(watch_time_weekday_t weekday)
{
    static const char *const names[] = {
        "", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY", "SUNDAY",
    };

    return weekday <= WATCH_TIME_WEEKDAY_SUNDAY ? names[weekday] : "";
}

static const char *watch_ui_theme_month(uint8_t month)
{
    static const char *const names[] = {
        "", "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT",
        "NOV", "DEC",
    };

    return month <= 12U ? names[month] : "";
}

static bool watch_ui_theme_render_watchface(lv_obj_t *screen, const watch_snapshot_t *snapshot,
                                            const watch_ui_palette_t *palette)
{
    char date_text[16] = "DATE --";
    char battery_text[24] = "BATTERY --";
    const char *steps_text = "--";
    lv_obj_t *date;
    lv_obj_t *weekday;
    lv_obj_t *battery;
    lv_obj_t *steps;
    lv_obj_t *status;
    lv_obj_t *hint;
    const bool degraded = snapshot->sensor_snapshot.degraded;

    date = watch_ui_theme_find_label(screen, "watchface_date");
    weekday = watch_ui_theme_find_label(screen, "watchface_weekday");
    battery = watch_ui_theme_find_label(screen, "watchface_battery");
    steps = watch_ui_theme_find_label(screen, "watchface_steps");
    status = watch_ui_theme_find_label(screen, "watchface_status");
    hint = watch_ui_theme_find_label(screen, "page_hint");
    if (date == NULL || weekday == NULL || battery == NULL || steps == NULL || status == NULL
        || hint == NULL) {
        return false;
    }

    if (snapshot->time_valid) {
        (void)snprintf(date_text, sizeof(date_text), "%s %02u", watch_ui_theme_month(snapshot->time.month),
                       snapshot->time.day);
        (void)snprintf(battery_text, sizeof(battery_text), "BATTERY --");
    }
    watch_ui_theme_set_text(date, date_text, palette, palette->text);
    watch_ui_theme_set_text(weekday, watch_ui_theme_weekday(snapshot->time.weekday), palette,
                            palette->muted);
    watch_ui_theme_set_text(battery, battery_text, palette, palette->degraded);
    watch_ui_theme_set_text(steps, steps_text, palette, palette->text);
    watch_ui_theme_set_text(status, degraded ? "DEGRADED" : "READY", palette,
                            degraded ? palette->degraded : palette->success);

    watch_ui_theme_style_label(date, palette, palette->text, WATCH_UI_CONTENT_WIDTH, WATCH_UI_MARGIN,
                               58);
    watch_ui_theme_style_label(weekday, palette, palette->muted, WATCH_UI_CONTENT_WIDTH,
                               WATCH_UI_MARGIN, 82);
    watch_ui_theme_style_label(battery, palette, palette->degraded, WATCH_UI_CONTENT_WIDTH,
                               WATCH_UI_MARGIN, 206);
    watch_ui_theme_style_label(steps, palette,
                               snapshot->sensor_snapshot.degraded ? palette->degraded : palette->success,
                               WATCH_UI_CONTENT_WIDTH, WATCH_UI_MARGIN, 226);
    watch_ui_theme_style_label(status, palette, palette->muted, WATCH_UI_CONTENT_WIDTH, WATCH_UI_MARGIN,
                               246);
    return true;
}

static bool watch_ui_theme_render_launcher(lv_obj_t *screen, const watch_snapshot_t *snapshot,
                                            const watch_ui_palette_t *palette)
{
    static const char *const icon_text[] = { "ST", "TM", "CA", "SE" };
    static const char *const app_names[] = { "STATUS", "TIMER", "CALENDAR", "SETTINGS" };
    static const char *const card_names[] = {
        "launcher_item_status", "launcher_item_timer", "launcher_item_calendar",
        "launcher_item_settings",
    };
    static const char *const label_names[] = {
        "launcher_label_status", "launcher_label_timer", "launcher_label_calendar",
        "launcher_label_settings",
    };
    lv_obj_t *list = watch_ui_theme_find_object(screen, "launcher_list");
    uint8_t index;

    if (list == NULL) {
        return false;
    }

    for (index = 0U; index < 4U; ++index) {
        lv_obj_t *card;
        lv_obj_t *label;
        const watch_app_entry_t *app = watch_core_get_launcher_app(index);

        if (app == NULL) {
            return false;
        }

        card = watch_ui_theme_find_object(list, card_names[index]);
        if (card == NULL) {
            return false;
        }
        label = watch_ui_theme_find_label(card, label_names[index]);
        if (label == NULL) {
            return false;
        }

        lv_obj_set_pos(card, 0, (lv_coord_t)(index * (WATCH_UI_ROW_HEIGHT + WATCH_UI_ROW_GAP)));
        watch_ui_theme_style_card(card, palette, index == snapshot->launcher_index);
        {
            char row_text[32];
            (void)snprintf(row_text, sizeof(row_text), "[%s] %s", icon_text[index],
                           app->id < WATCH_APP_COUNT ? app_names[app->id] : "APP");
            watch_ui_theme_set_text(label, row_text, palette, palette->text);
        }
        watch_ui_theme_style_label(label, palette, palette->text, 190, 12, 10);
    }

    return true;
}

static bool watch_ui_theme_render_settings(lv_obj_t *screen, const watch_ui_palette_t *palette)
{
    static const char *const card_names[] = {
        "settings_item_theme", "settings_item_brightness", "settings_item_time_format",
    };
    static const char *const title_names[] = {
        "settings_title_theme", "settings_title_brightness", "settings_title_time_format",
    };
    static const char *const value_names[] = {
        "settings_value_theme", "settings_value_brightness", "settings_value_time_format",
    };
    static const char *const titles[] = { "THEME", "BRIGHTNESS", "TIME FORMAT" };
    static const char *const values[] = { "DARK", "80%", "24 H" };
    lv_obj_t *list = watch_ui_theme_find_object(screen, "settings_list");
    uint8_t index;

    if (list == NULL) {
        return false;
    }

    for (index = 0U; index < 3U; ++index) {
        lv_obj_t *card;
        lv_obj_t *title;
        lv_obj_t *value;

        card = watch_ui_theme_find_object(list, card_names[index]);
        if (card == NULL) {
            return false;
        }
        title = watch_ui_theme_find_label(card, title_names[index]);
        value = watch_ui_theme_find_label(card, value_names[index]);
        if (title == NULL || value == NULL) {
            return false;
        }

        lv_obj_set_pos(card, 0, (lv_coord_t)(index * (WATCH_UI_ROW_HEIGHT + WATCH_UI_ROW_GAP)));
        watch_ui_theme_style_card(card, palette, false);
        watch_ui_theme_set_text(title, titles[index], palette, palette->text);
        watch_ui_theme_set_text(value, values[index], palette, palette->accent);
        watch_ui_theme_style_label(title, palette, palette->text, 132, 12, 10);
        watch_ui_theme_style_label(value, palette, palette->accent, 62, 142, 10);
    }

    return true;
}

static bool watch_ui_theme_render_status(lv_obj_t *screen, const watch_snapshot_t *snapshot,
                                         const watch_ui_palette_t *palette)
{
    lv_obj_t *summary = watch_ui_theme_find_label(screen, "status_summary");
    lv_obj_t *time = watch_ui_theme_find_label(screen, "status_time");
    lv_obj_t *battery = watch_ui_theme_find_label(screen, "status_battery");
    lv_obj_t *sensor = watch_ui_theme_find_label(screen, "status_sensors");
    lv_obj_t *storage = watch_ui_theme_find_label(screen, "status_storage");
    lv_obj_t *input = watch_ui_theme_find_label(screen, "status_input");
    lv_obj_t *reason = watch_ui_theme_find_label(screen, "status_reason");

    if ((summary == NULL && (time == NULL || battery == NULL)) || sensor == NULL || storage == NULL
        || input == NULL) {
        return false;
    }

    if (summary != NULL) {
        watch_ui_theme_set_text(summary, "SYSTEM STATUS", palette, palette->accent);
    }
    if (time != NULL) {
        char time_text[24] = "TIME  --:--:--";
        if (snapshot->time_valid) {
            (void)snprintf(time_text, sizeof(time_text), "TIME  %02u:%02u:%02u",
                           snapshot->time.hour, snapshot->time.minute, snapshot->time.second);
        }
        watch_ui_theme_set_text(time, time_text, palette, palette->text);
    }
    if (battery != NULL) {
        watch_ui_theme_set_text(battery, "BATTERY  --", palette, palette->text);
    }
    watch_ui_theme_set_text(sensor, snapshot->sensor_snapshot.degraded ? "SENSORS  DEGRADED"
                                                                        : "SENSORS  READY",
                            palette, snapshot->sensor_snapshot.degraded ? palette->degraded
                                                                         : palette->success);
    watch_ui_theme_set_text(storage, "STORAGE  NOT REPORTED", palette, palette->muted);
    watch_ui_theme_set_text(input, "INPUT  CST816 / ENCODER", palette, palette->muted);
    if (summary != NULL) {
        watch_ui_theme_style_label(summary, palette, palette->accent, WATCH_UI_CONTENT_WIDTH,
                                   WATCH_UI_MARGIN, 60);
    }
    watch_ui_theme_style_label(sensor, palette,
                               snapshot->sensor_snapshot.degraded ? palette->degraded : palette->success,
                               WATCH_UI_CONTENT_WIDTH, WATCH_UI_MARGIN, 104);
    watch_ui_theme_style_label(storage, palette, palette->muted, WATCH_UI_CONTENT_WIDTH,
                               WATCH_UI_MARGIN, 140);
    if (reason != NULL) {
        watch_ui_theme_set_text(reason,
                                snapshot->sensor_snapshot.degraded ? "SENSOR STATUS: DEGRADED"
                                                                    : "SENSOR STATUS: READY",
                                palette,
                                snapshot->sensor_snapshot.degraded ? palette->degraded
                                                                    : palette->success);
    }
    return true;
}

static bool watch_ui_theme_render_simple_page(lv_obj_t *screen, watch_page_t page,
                                              const watch_snapshot_t *snapshot,
                                              const watch_ui_palette_t *palette)
{
    const char *value = page == WATCH_PAGE_TIMER ? "00:00.0" : "CALENDAR READY";
    lv_obj_t *detail = watch_ui_theme_find_label(screen,
                                                 page == WATCH_PAGE_TIMER ? "timer_value"
                                                                          : "calendar_value");

    if (page == WATCH_PAGE_CALENDAR && detail == NULL) {
        detail = watch_ui_theme_find_label(screen, "calendar_month");
    }

    if (detail == NULL) {
        return false;
    }

    if (page == WATCH_PAGE_CALENDAR && snapshot->time_valid) {
        static char calendar_text[24];
        (void)snprintf(calendar_text, sizeof(calendar_text), "%04u-%02u", snapshot->time.year,
                       snapshot->time.month);
        value = calendar_text;
    }

    watch_ui_theme_set_text(detail, value, palette, palette->accent);
    watch_ui_theme_style_label(detail, palette, palette->accent, WATCH_UI_CONTENT_WIDTH,
                               WATCH_UI_MARGIN, 100);
    return true;
}

const watch_ui_palette_t *watch_ui_theme_palette(watch_ui_theme_mode_t mode)
{
    return mode == WATCH_UI_THEME_LIGHT ? &s_light_palette : &s_dark_palette;
}

watch_ui_theme_mode_t watch_ui_theme_get_mode(void)
{
    return s_theme_mode;
}

bool watch_ui_theme_set_mode(watch_ui_theme_mode_t mode)
{
    if (mode >= WATCH_UI_THEME_COUNT) {
        return false;
    }

    s_theme_mode = mode;
    return true;
}

bool watch_ui_theme_apply(lv_obj_t *screen, const watch_snapshot_t *snapshot)
{
    const watch_ui_palette_t *palette;

    if (screen == NULL || snapshot == NULL || snapshot->page >= WATCH_PAGE_COUNT) {
        return false;
    }

    palette = watch_ui_theme_palette(s_theme_mode);
    watch_ui_theme_style_frame(screen, palette);
    switch (snapshot->page) {
    case WATCH_PAGE_WATCHFACE:
        return watch_ui_theme_render_watchface(screen, snapshot, palette);
    case WATCH_PAGE_LAUNCHER:
        return watch_ui_theme_render_launcher(screen, snapshot, palette);
    case WATCH_PAGE_STATUS:
        return watch_ui_theme_render_status(screen, snapshot, palette);
    case WATCH_PAGE_SETTINGS:
        return watch_ui_theme_render_settings(screen, palette);
    case WATCH_PAGE_TIMER:
    case WATCH_PAGE_CALENDAR:
        return watch_ui_theme_render_simple_page(screen, snapshot->page, snapshot, palette);
    case WATCH_PAGE_RESOURCES:
    case WATCH_PAGE_DIAGNOSTICS:
    case WATCH_PAGE_COUNT:
        return true;
    }

    return false;
}
