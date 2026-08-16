#include <assert.h>

#include "watch_core.h"

int main(void)
{
    watch_command_t command;
    watch_core_t core;
    watch_snapshot_t snapshot;

    assert(watch_core_init(&core));
    assert(watch_core_read_snapshot(&core, &snapshot));
    assert(snapshot.page == WATCH_PAGE_DIAGNOSTICS);
    assert(snapshot.page_depth == 1U);
    assert(!snapshot.popup_visible);
    assert(snapshot.revision == 1U);

    assert(watch_core_dispatch_event(&core, &(watch_event_t) { .type = WATCH_EVENT_SELECT }));
    assert(watch_core_read_snapshot(&core, &snapshot));
    assert(snapshot.page == WATCH_PAGE_DIAGNOSTICS);
    assert(snapshot.page_depth == 1U);
    assert(snapshot.popup_visible);
    assert(snapshot.revision == 2U);
    assert(watch_core_take_command(&core, &command));
    assert(command.type == WATCH_COMMAND_POPUP_CHANGED);
    assert(command.page == WATCH_PAGE_DIAGNOSTICS);
    assert(command.popup_visible);

    assert(watch_core_dispatch_event(&core, &(watch_event_t) { .type = WATCH_EVENT_BACK }));
    assert(watch_core_read_snapshot(&core, &snapshot));
    assert(snapshot.page == WATCH_PAGE_DIAGNOSTICS);
    assert(snapshot.page_depth == 1U);
    assert(!snapshot.popup_visible);
    assert(snapshot.revision == 3U);
    assert(watch_core_take_command(&core, &command));
    assert(command.type == WATCH_COMMAND_POPUP_CHANGED);
    assert(command.page == WATCH_PAGE_DIAGNOSTICS);
    assert(!command.popup_visible);

    assert(watch_core_dispatch_event(&core, &(watch_event_t) { .type = WATCH_EVENT_BACK }));
    assert(watch_core_read_snapshot(&core, &snapshot));
    assert(snapshot.page == WATCH_PAGE_WATCHFACE);
    assert(snapshot.page_depth == 0U);
    assert(!snapshot.popup_visible);
    assert(snapshot.revision == 4U);
    assert(watch_core_take_command(&core, &command));
    assert(command.type == WATCH_COMMAND_PAGE_CHANGED);
    assert(command.page == WATCH_PAGE_WATCHFACE);
    assert(!command.popup_visible);

    return 0;
}
