#include "watch_core.h"

#include <stddef.h>

static const watch_app_entry_t s_apps[WATCH_APP_COUNT] = {
    { WATCH_APP_STATUS, WATCH_PAGE_STATUS, "app.status", "status", 0U, true },
    { WATCH_APP_TIMER, WATCH_PAGE_TIMER, "app.timer", "timer", 0U, true },
    { WATCH_APP_CALENDAR, WATCH_PAGE_CALENDAR, "app.calendar", "calendar", 0U, true },
    { WATCH_APP_SETTINGS, WATCH_PAGE_SETTINGS, "app.settings", "settings", 0U, true },
    { WATCH_APP_RESOURCES, WATCH_PAGE_RESOURCES, "app.resources", "resources",
      WATCH_APP_CAPABILITY_SYSTEM | WATCH_APP_CAPABILITY_HIDDEN, false },
    { WATCH_APP_DIAGNOSTICS, WATCH_PAGE_DIAGNOSTICS, "app.diagnostics", "diagnostics",
      WATCH_APP_CAPABILITY_SYSTEM | WATCH_APP_CAPABILITY_HIDDEN, false },
};

const watch_app_entry_t *watch_core_get_app(watch_app_id_t app_id)
{
    if (app_id >= WATCH_APP_COUNT) {
        return NULL;
    }

    return &s_apps[app_id];
}

const watch_app_entry_t *watch_core_get_launcher_app(uint8_t launcher_index)
{
    uint8_t visible_index = 0U;
    uint8_t app_index;

    for (app_index = 0U; app_index < WATCH_APP_COUNT; ++app_index) {
        if (!s_apps[app_index].visible) {
            continue;
        }
        if (visible_index == launcher_index) {
            return &s_apps[app_index];
        }
        ++visible_index;
    }

    return NULL;
}

uint8_t watch_core_launcher_count(void)
{
    uint8_t count = 0U;
    uint8_t app_index;

    for (app_index = 0U; app_index < WATCH_APP_COUNT; ++app_index) {
        if (s_apps[app_index].visible) {
            ++count;
        }
    }
    return count;
}

static bool watch_core_has_command_space(const watch_core_t *core)
{
    return core->command_count < WATCH_CORE_COMMAND_CAPACITY;
}

static void watch_core_enqueue_command(watch_core_t *core, watch_command_type_t type)
{
    watch_command_t *command = &core->command_queue[core->command_head];

    command->type = type;
    command->page = core->page;
    command->launcher_index = core->launcher_index;
    command->popup_visible = core->popup_visible;
    command->time_valid = core->time_valid;
    command->time = core->time;
    command->sensor_snapshot = core->sensor_snapshot;
    command->revision = core->revision;
    core->command_head = (uint8_t)((core->command_head + 1U) % WATCH_CORE_COMMAND_CAPACITY);
    core->command_count++;
}

static void watch_core_commit_change(watch_core_t *core, watch_command_type_t type)
{
    core->revision++;
    watch_core_enqueue_command(core, type);
}

static bool watch_core_set_popup_visible(watch_core_t *core, bool popup_visible)
{
    if (core->popup_visible == popup_visible) {
        return true;
    }

    if (!watch_core_has_command_space(core)) {
        return false;
    }

    core->popup_visible = popup_visible;
    watch_core_commit_change(core, WATCH_COMMAND_POPUP_CHANGED);
    return true;
}

static bool watch_core_set_time(watch_core_t *core, const watch_time_value_t *time)
{
    if (!watch_time_is_valid(time)) {
        return false;
    }

    if (core->time_valid && watch_time_equal(&core->time, time)) {
        return true;
    }

    if (!watch_core_has_command_space(core)) {
        return false;
    }

    core->time = *time;
    core->time_valid = true;
    watch_core_commit_change(core, WATCH_COMMAND_TIME_CHANGED);
    return true;
}

static bool watch_core_set_sensor_snapshot(watch_core_t *core,
                                           const watch_sensor_aggregate_snapshot_t *snapshot)
{
    if (!watch_sensor_aggregate_snapshot_is_valid(snapshot)) {
        return false;
    }

    if (watch_sensor_aggregate_snapshot_equal(&core->sensor_snapshot, snapshot)) {
        return true;
    }

    if (!watch_core_has_command_space(core)) {
        return false;
    }

    core->sensor_snapshot = *snapshot;
    watch_core_commit_change(core, WATCH_COMMAND_SENSOR_STATUS_CHANGED);
    return true;
}

static bool watch_core_open_page(watch_core_t *core, watch_page_t page)
{
    if (core->page_depth >= WATCH_CORE_PAGE_STACK_CAPACITY || !watch_core_has_command_space(core)) {
        return false;
    }

    core->page_stack[core->page_depth] = core->page;
    core->page_depth++;
    core->page = page;
    core->popup_visible = false;
    watch_core_commit_change(core, WATCH_COMMAND_PAGE_CHANGED);
    return true;
}

static bool watch_core_select_launcher_item(watch_core_t *core, uint8_t launcher_index)
{
    const watch_app_entry_t *app;

    if (core->page != WATCH_PAGE_LAUNCHER) {
        return true;
    }

    app = watch_core_get_launcher_app(launcher_index);

    if (app == NULL) {
        return false;
    }

    if (core->page_depth >= WATCH_CORE_PAGE_STACK_CAPACITY || !watch_core_has_command_space(core)) {
        return false;
    }

    core->launcher_index = launcher_index;
    return watch_core_open_page(core, app->page);
}

static bool watch_core_move_selection(watch_core_t *core, int8_t direction)
{
    uint8_t next_index;

    if (!watch_core_has_command_space(core)) {
        return false;
    }

    if (direction > 0) {
        next_index = (uint8_t)((core->launcher_index + 1U) % watch_core_launcher_count());
    } else {
        next_index = (core->launcher_index == 0U) ? (watch_core_launcher_count() - 1U)
                                                  : (uint8_t)(core->launcher_index - 1U);
    }

    if (next_index == core->launcher_index) {
        return true;
    }

    core->launcher_index = next_index;
    watch_core_commit_change(core, WATCH_COMMAND_SELECTION_CHANGED);
    return true;
}

static bool watch_core_go_back(watch_core_t *core)
{
    if (core->page_depth == 0U) {
        return true;
    }

    if (!watch_core_has_command_space(core)) {
        return false;
    }

    core->page_depth--;
    core->page = core->page_stack[core->page_depth];
    core->popup_visible = false;
    watch_core_commit_change(core, WATCH_COMMAND_PAGE_CHANGED);
    return true;
}

bool watch_core_init(watch_core_t *core)
{
    if (core == NULL) {
        return false;
    }

    *core = (watch_core_t) { 0 };
    core->sensor_snapshot.degraded = true;
#if defined(WATCH_DIAGNOSTIC)
    core->page_stack[0] = WATCH_PAGE_WATCHFACE;
    core->page = WATCH_PAGE_DIAGNOSTICS;
    core->page_depth = 1U;
    core->revision = 1U;
#else
    core->page = WATCH_PAGE_WATCHFACE;
#endif
    return true;
}

bool watch_core_dispatch_event(watch_core_t *core, const watch_event_t *event)
{
    if (core == NULL || event == NULL || event->type <= WATCH_EVENT_NONE
        || event->type >= WATCH_EVENT_COUNT) {
        return false;
    }

    switch (event->type) {
    case WATCH_EVENT_WAKE:
        /* Power ownership is deferred; keep WAKE in the normalized contract. */
        return true;
    case WATCH_EVENT_BACK:
        if (core->popup_visible) {
            return watch_core_set_popup_visible(core, false);
        }
        return watch_core_go_back(core);
    case WATCH_EVENT_SELECT:
        if (core->page == WATCH_PAGE_DIAGNOSTICS) {
            return watch_core_set_popup_visible(core, true);
        }
        return true;
    case WATCH_EVENT_ENCODER_PRESS:
        if (core->page == WATCH_PAGE_WATCHFACE) {
            return watch_core_open_page(core, WATCH_PAGE_LAUNCHER);
        }
        if (!watch_core_has_command_space(core)) {
            return false;
        }
        core->popup_visible = false;
        core->page_depth = 0U;
        core->page = WATCH_PAGE_WATCHFACE;
        watch_core_commit_change(core, WATCH_COMMAND_PAGE_CHANGED);
        return true;
    case WATCH_EVENT_UP:
        if (core->page != WATCH_PAGE_LAUNCHER) {
            return true;
        }
        return watch_core_move_selection(core, -1);
    case WATCH_EVENT_DOWN:
        if (core->page != WATCH_PAGE_LAUNCHER) {
            return true;
        }
        return watch_core_move_selection(core, 1);
    case WATCH_EVENT_LAUNCHER_ITEM_TAPPED:
        return watch_core_select_launcher_item(core, event->launcher_index);
    case WATCH_EVENT_TIME_UPDATED:
        return watch_core_set_time(core, &event->time);
    case WATCH_EVENT_SENSOR_STATUS_UPDATED:
        return watch_core_set_sensor_snapshot(core, &event->sensor_snapshot);
    case WATCH_EVENT_NONE:
    case WATCH_EVENT_COUNT:
        return false;
    }

    return false;
}

bool watch_core_read_snapshot(const watch_core_t *core, watch_snapshot_t *snapshot)
{
    if (core == NULL || snapshot == NULL) {
        return false;
    }

    snapshot->page = core->page;
    snapshot->page_depth = core->page_depth;
    snapshot->launcher_index = core->launcher_index;
    snapshot->popup_visible = core->popup_visible;
    snapshot->time_valid = core->time_valid;
    snapshot->time = core->time;
    snapshot->sensor_snapshot = core->sensor_snapshot;
    snapshot->revision = core->revision;
    return true;
}

bool watch_core_take_command(watch_core_t *core, watch_command_t *command)
{
    if (core == NULL || command == NULL || core->command_count == 0U) {
        return false;
    }

    *command = core->command_queue[core->command_tail];
    core->command_tail = (uint8_t)((core->command_tail + 1U) % WATCH_CORE_COMMAND_CAPACITY);
    core->command_count--;
    return true;
}
