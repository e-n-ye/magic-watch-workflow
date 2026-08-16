#ifndef WATCH_RUNTIME_H
#define WATCH_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>

#define WATCH_RUNTIME_UI_EVENT_QUEUE_CAPACITY 8U
#define WATCH_RUNTIME_HEARTBEAT_TIMEOUT_MS 2000U

typedef enum {
    WATCH_RUNTIME_INIT_RESET = 0,
    WATCH_RUNTIME_INIT_CORE,
    WATCH_RUNTIME_INIT_INPUT,
    WATCH_RUNTIME_INIT_RUNNING,
    WATCH_RUNTIME_INIT_FAILED,
    WATCH_RUNTIME_INIT_COUNT
} watch_runtime_init_stage_t;

typedef enum {
    WATCH_RUNTIME_SERVICE_APP = 0,
    WATCH_RUNTIME_SERVICE_UI,
    WATCH_RUNTIME_SERVICE_USB,
    WATCH_RUNTIME_SERVICE_SENSOR,
    WATCH_RUNTIME_SERVICE_COUNT
} watch_runtime_service_t;

typedef enum {
    WATCH_RUNTIME_HEALTH_NOT_STARTED = 0,
    WATCH_RUNTIME_HEALTH_HEALTHY,
    WATCH_RUNTIME_HEALTH_STALE,
    WATCH_RUNTIME_HEALTH_COUNT
} watch_runtime_health_state_t;

typedef struct
{
    uint16_t type;
    uint32_t value;
    uint32_t timestamp_ms;
} watch_ui_event_t;

typedef struct
{
    watch_runtime_health_state_t state;
    uint32_t started_at_ms;
    uint32_t last_heartbeat_ms;
    uint32_t heartbeat_count;
} watch_runtime_health_t;

bool watch_runtime_init(uint32_t now_ms);
bool watch_runtime_advance_init(watch_runtime_init_stage_t next_stage);
void watch_runtime_fail(void);
watch_runtime_init_stage_t watch_runtime_init_stage(void);
bool watch_runtime_is_ready(void);

uint32_t watch_runtime_elapsed_ms(uint32_t now_ms, uint32_t previous_ms);

bool watch_runtime_start_service(watch_runtime_service_t service, uint32_t now_ms);
bool watch_runtime_heartbeat(watch_runtime_service_t service, uint32_t now_ms);
bool watch_runtime_read_health(watch_runtime_service_t service, uint32_t now_ms,
                               watch_runtime_health_t *health);

/* Task-context services post events; the UI task is the sole consumer. */
bool watch_runtime_post_ui_event(const watch_ui_event_t *event);
bool watch_runtime_take_ui_event(watch_ui_event_t *event);
uint8_t watch_runtime_ui_event_count(void);

#endif /* WATCH_RUNTIME_H */
