#ifndef WATCH_CORE_H
#define WATCH_CORE_H

#include <stdbool.h>
#include <stdint.h>

#include "watch_sensor_aggregate.h"
#include "watch_time.h"

#define WATCH_CORE_COMMAND_CAPACITY 8U
#define WATCH_CORE_PAGE_STACK_CAPACITY 4U
#define WATCH_CORE_LAUNCHER_ITEM_COUNT 4U
#define WATCH_CORE_APP_COUNT 6U
#define WATCH_APP_CAPABILITY_NONE 0U
#define WATCH_APP_CAPABILITY_SYSTEM (1UL << 0)
#define WATCH_APP_CAPABILITY_HIDDEN (1UL << 1)

typedef enum {
    WATCH_PAGE_WATCHFACE = 0,
    WATCH_PAGE_LAUNCHER,
    WATCH_PAGE_STATUS,
    WATCH_PAGE_TIMER,
    WATCH_PAGE_CALENDAR,
    WATCH_PAGE_SETTINGS,
    WATCH_PAGE_RESOURCES,
    WATCH_PAGE_DIAGNOSTICS,
    WATCH_PAGE_COUNT
} watch_page_t;

typedef enum {
    WATCH_EVENT_NONE = 0,
    WATCH_EVENT_WAKE,
    WATCH_EVENT_BACK,
    WATCH_EVENT_SELECT,
    WATCH_EVENT_ENCODER_PRESS,
    WATCH_EVENT_UP,
    WATCH_EVENT_DOWN,
    WATCH_EVENT_TIME_UPDATED,
    WATCH_EVENT_SENSOR_STATUS_UPDATED,
    WATCH_EVENT_COUNT
} watch_event_type_t;

typedef enum {
    WATCH_APP_STATUS = 0,
    WATCH_APP_TIMER,
    WATCH_APP_CALENDAR,
    WATCH_APP_SETTINGS,
    WATCH_APP_RESOURCES,
    WATCH_APP_DIAGNOSTICS,
    WATCH_APP_COUNT
} watch_app_id_t;

typedef struct
{
    watch_app_id_t id;
    watch_page_t page;
    const char *translation_key;
    const char *icon_key;
    uint32_t capabilities;
    bool visible;
} watch_app_entry_t;

typedef struct
{
    watch_event_type_t type;
    watch_time_value_t time;
    watch_sensor_aggregate_snapshot_t sensor_snapshot;
} watch_event_t;

typedef enum {
    WATCH_COMMAND_NONE = 0,
    WATCH_COMMAND_PAGE_CHANGED,
    WATCH_COMMAND_SELECTION_CHANGED,
    WATCH_COMMAND_POPUP_CHANGED,
    WATCH_COMMAND_TIME_CHANGED,
    WATCH_COMMAND_SENSOR_STATUS_CHANGED,
    WATCH_COMMAND_COUNT
} watch_command_type_t;

typedef struct
{
    watch_command_type_t type;
    watch_page_t page;
    uint8_t launcher_index;
    bool popup_visible;
    bool time_valid;
    watch_time_value_t time;
    watch_sensor_aggregate_snapshot_t sensor_snapshot;
    uint32_t revision;
} watch_command_t;

typedef struct
{
    watch_page_t page;
    uint8_t page_depth;
    uint8_t launcher_index;
    bool popup_visible;
    bool time_valid;
    watch_time_value_t time;
    watch_sensor_aggregate_snapshot_t sensor_snapshot;
    uint32_t revision;
} watch_snapshot_t;

typedef struct
{
    watch_page_t page_stack[WATCH_CORE_PAGE_STACK_CAPACITY];
    watch_command_t command_queue[WATCH_CORE_COMMAND_CAPACITY];
    watch_page_t page;
    uint8_t page_depth;
    uint8_t launcher_index;
    bool popup_visible;
    bool time_valid;
    watch_time_value_t time;
    watch_sensor_aggregate_snapshot_t sensor_snapshot;
    uint8_t command_head;
    uint8_t command_tail;
    uint8_t command_count;
    uint32_t revision;
} watch_core_t;

bool watch_core_init(watch_core_t *core);
bool watch_core_dispatch_event(watch_core_t *core, const watch_event_t *event);
bool watch_core_read_snapshot(const watch_core_t *core, watch_snapshot_t *snapshot);
bool watch_core_take_command(watch_core_t *core, watch_command_t *command);
const watch_app_entry_t *watch_core_get_app(watch_app_id_t app_id);
const watch_app_entry_t *watch_core_get_launcher_app(uint8_t launcher_index);
uint8_t watch_core_launcher_count(void);

#endif /* WATCH_CORE_H */
