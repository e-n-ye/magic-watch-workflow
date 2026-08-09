#ifndef WATCH_CORE_H
#define WATCH_CORE_H

#include <stdbool.h>
#include <stdint.h>

#define WATCH_CORE_COMMAND_CAPACITY 8U
#define WATCH_CORE_PAGE_STACK_CAPACITY 4U
#define WATCH_CORE_LAUNCHER_ITEM_COUNT 2U

typedef enum {
    WATCH_PAGE_WATCHFACE = 0,
    WATCH_PAGE_LAUNCHER,
    WATCH_PAGE_STATUS,
    WATCH_PAGE_SETTINGS,
    WATCH_PAGE_COUNT
} watch_page_t;

typedef enum {
    WATCH_EVENT_NONE = 0,
    WATCH_EVENT_WAKE,
    WATCH_EVENT_BACK,
    WATCH_EVENT_SELECT,
    WATCH_EVENT_UP,
    WATCH_EVENT_DOWN,
    WATCH_EVENT_COUNT
} watch_event_type_t;

typedef struct
{
    watch_event_type_t type;
} watch_event_t;

typedef enum {
    WATCH_COMMAND_NONE = 0,
    WATCH_COMMAND_PAGE_CHANGED,
    WATCH_COMMAND_SELECTION_CHANGED,
    WATCH_COMMAND_COUNT
} watch_command_type_t;

typedef struct
{
    watch_command_type_t type;
    watch_page_t page;
    uint8_t launcher_index;
    uint32_t revision;
} watch_command_t;

typedef struct
{
    watch_page_t page;
    uint8_t page_depth;
    uint8_t launcher_index;
    uint32_t revision;
} watch_snapshot_t;

typedef struct
{
    watch_page_t page_stack[WATCH_CORE_PAGE_STACK_CAPACITY];
    watch_command_t command_queue[WATCH_CORE_COMMAND_CAPACITY];
    watch_page_t page;
    uint8_t page_depth;
    uint8_t launcher_index;
    uint8_t command_head;
    uint8_t command_tail;
    uint8_t command_count;
    uint32_t revision;
} watch_core_t;

bool watch_core_init(watch_core_t *core);
bool watch_core_dispatch_event(watch_core_t *core, const watch_event_t *event);
bool watch_core_read_snapshot(const watch_core_t *core, watch_snapshot_t *snapshot);
bool watch_core_take_command(watch_core_t *core, watch_command_t *command);

#endif /* WATCH_CORE_H */
