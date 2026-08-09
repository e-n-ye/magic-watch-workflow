#include "watch_core.h"

#include <stddef.h>

static const watch_page_t s_launcher_pages[WATCH_CORE_LAUNCHER_ITEM_COUNT] = {
    WATCH_PAGE_STATUS,
    WATCH_PAGE_SETTINGS,
};

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
    command->revision = core->revision;
    core->command_head = (uint8_t)((core->command_head + 1U) % WATCH_CORE_COMMAND_CAPACITY);
    core->command_count++;
}

static void watch_core_commit_change(watch_core_t *core, watch_command_type_t type)
{
    core->revision++;
    watch_core_enqueue_command(core, type);
}

static bool watch_core_open_page(watch_core_t *core, watch_page_t page)
{
    if (core->page_depth >= WATCH_CORE_PAGE_STACK_CAPACITY || !watch_core_has_command_space(core)) {
        return false;
    }

    core->page_stack[core->page_depth] = core->page;
    core->page_depth++;
    core->page = page;
    watch_core_commit_change(core, WATCH_COMMAND_PAGE_CHANGED);
    return true;
}

static bool watch_core_select_launcher_item(watch_core_t *core)
{
    if (core->launcher_index >= WATCH_CORE_LAUNCHER_ITEM_COUNT) {
        return false;
    }

    return watch_core_open_page(core, s_launcher_pages[core->launcher_index]);
}

static bool watch_core_move_selection(watch_core_t *core, int8_t direction)
{
    uint8_t next_index;

    if (!watch_core_has_command_space(core)) {
        return false;
    }

    if (direction > 0) {
        next_index = (uint8_t)((core->launcher_index + 1U) % WATCH_CORE_LAUNCHER_ITEM_COUNT);
    } else {
        next_index = (core->launcher_index == 0U) ? (WATCH_CORE_LAUNCHER_ITEM_COUNT - 1U)
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
    watch_core_commit_change(core, WATCH_COMMAND_PAGE_CHANGED);
    return true;
}

bool watch_core_init(watch_core_t *core)
{
    if (core == NULL) {
        return false;
    }

    *core = (watch_core_t) { 0 };
    core->page = WATCH_PAGE_WATCHFACE;
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
        return watch_core_go_back(core);
    case WATCH_EVENT_SELECT:
        if (core->page == WATCH_PAGE_WATCHFACE) {
            return watch_core_open_page(core, WATCH_PAGE_LAUNCHER);
        }
        if (core->page == WATCH_PAGE_LAUNCHER) {
            return watch_core_select_launcher_item(core);
        }
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
