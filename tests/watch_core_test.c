#include "watch_core.h"

#include <assert.h>

static watch_event_t watch_test_event(watch_event_type_t type)
{
    return (watch_event_t){.type = type};
}

static bool watch_test_dispatch(watch_core_t *core, watch_event_type_t type)
{
    watch_event_t event = watch_test_event(type);

    return watch_core_dispatch_event(core, &event);
}

static void test_initial_state(void)
{
    watch_core_t core;
    watch_snapshot_t snapshot;

    assert(watch_core_init(&core));
    assert(watch_core_read_snapshot(&core, &snapshot));
    assert(snapshot.page == WATCH_PAGE_WATCHFACE);
    assert(snapshot.page_depth == 0U);
    assert(snapshot.launcher_index == 0U);
    assert(snapshot.revision == 0U);
    assert(!watch_core_take_command(&core, &(watch_command_t){0}));
}

static void test_navigation_and_commands(void)
{
    watch_core_t core;
    watch_snapshot_t snapshot;
    watch_command_t command;

    assert(watch_core_init(&core));

    assert(watch_test_dispatch(&core, WATCH_EVENT_SELECT));
    assert(watch_core_read_snapshot(&core, &snapshot));
    assert(snapshot.page == WATCH_PAGE_LAUNCHER);
    assert(snapshot.page_depth == 1U);
    assert(snapshot.revision == 1U);
    assert(watch_core_take_command(&core, &command));
    assert(command.type == WATCH_COMMAND_PAGE_CHANGED);
    assert(command.page == WATCH_PAGE_LAUNCHER);
    assert(command.revision == 1U);

    assert(watch_test_dispatch(&core, WATCH_EVENT_DOWN));
    assert(watch_core_read_snapshot(&core, &snapshot));
    assert(snapshot.launcher_index == 1U);
    assert(watch_core_take_command(&core, &command));
    assert(command.type == WATCH_COMMAND_SELECTION_CHANGED);
    assert(command.launcher_index == 1U);

    assert(watch_test_dispatch(&core, WATCH_EVENT_SELECT));
    assert(watch_core_read_snapshot(&core, &snapshot));
    assert(snapshot.page == WATCH_PAGE_SETTINGS);
    assert(snapshot.page_depth == 2U);
    assert(watch_core_take_command(&core, &command));
    assert(command.type == WATCH_COMMAND_PAGE_CHANGED);

    assert(watch_test_dispatch(&core, WATCH_EVENT_BACK));
    assert(watch_test_dispatch(&core, WATCH_EVENT_BACK));
    assert(watch_core_read_snapshot(&core, &snapshot));
    assert(snapshot.page == WATCH_PAGE_WATCHFACE);
    assert(snapshot.page_depth == 0U);
    assert(snapshot.revision == 5U);
}

static void test_ignored_events_and_validation(void)
{
    watch_core_t core;
    watch_snapshot_t before;
    watch_snapshot_t after;
    watch_event_t invalid_event = watch_test_event(WATCH_EVENT_NONE);

    assert(watch_core_init(&core));
    assert(watch_core_read_snapshot(&core, &before));
    assert(watch_test_dispatch(&core, WATCH_EVENT_WAKE));
    assert(watch_test_dispatch(&core, WATCH_EVENT_UP));
    assert(watch_test_dispatch(&core, WATCH_EVENT_BACK));
    assert(watch_core_read_snapshot(&core, &after));
    assert(after.page == before.page);
    assert(after.revision == before.revision);
    assert(!watch_core_dispatch_event(&core, &invalid_event));
    assert(!watch_core_read_snapshot(NULL, &after));
    assert(!watch_core_read_snapshot(&core, NULL));
}

static void test_command_queue_is_bounded(void)
{
    watch_core_t core;
    watch_snapshot_t snapshot;
    watch_snapshot_t before_full;
    watch_command_t command;

    assert(watch_core_init(&core));
    for (uint8_t index = 0U; index < WATCH_CORE_COMMAND_CAPACITY / 2U; index++) {
        assert(watch_test_dispatch(&core, WATCH_EVENT_SELECT));
        assert(watch_test_dispatch(&core, WATCH_EVENT_BACK));
    }

    assert(watch_core_read_snapshot(&core, &before_full));
    assert(!watch_test_dispatch(&core, WATCH_EVENT_SELECT));
    assert(watch_core_read_snapshot(&core, &snapshot));
    assert(snapshot.page == before_full.page);
    assert(snapshot.page_depth == before_full.page_depth);
    assert(snapshot.revision == before_full.revision);

    assert(watch_core_take_command(&core, &command));
    assert(watch_test_dispatch(&core, WATCH_EVENT_SELECT));
}

int main(void)
{
    test_initial_state();
    test_navigation_and_commands();
    test_ignored_events_and_validation();
    test_command_queue_is_bounded();
    return 0;
}
