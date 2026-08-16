#include "watch_runtime.h"

#include <stddef.h>
#include <stdatomic.h>

typedef struct
{
    bool started;
    uint32_t started_at_ms;
    uint32_t last_heartbeat_ms;
    uint32_t heartbeat_count;
} watch_runtime_health_entry_t;

typedef struct
{
    bool initialized;
    uint32_t initialized_at_ms;
    watch_runtime_init_stage_t init_stage;
    watch_ui_event_t ui_events[WATCH_RUNTIME_UI_EVENT_QUEUE_CAPACITY];
    uint8_t ui_event_head;
    uint8_t ui_event_tail;
    uint8_t ui_event_count;
    watch_runtime_health_entry_t health[WATCH_RUNTIME_SERVICE_COUNT];
} watch_runtime_state_t;

static watch_runtime_state_t s_runtime;
static atomic_flag s_ui_event_lock = ATOMIC_FLAG_INIT;

static bool watch_runtime_valid_service(watch_runtime_service_t service)
{
    return (uint32_t)service < WATCH_RUNTIME_SERVICE_COUNT;
}

static void watch_runtime_lock_ui_events(void)
{
    while (atomic_flag_test_and_set_explicit(&s_ui_event_lock, memory_order_acquire)) {
        /* Producers and the UI task hold this critical section only briefly. */
    }
}

static void watch_runtime_unlock_ui_events(void)
{
    atomic_flag_clear_explicit(&s_ui_event_lock, memory_order_release);
}

uint32_t watch_runtime_elapsed_ms(uint32_t now_ms, uint32_t previous_ms)
{
    return now_ms - previous_ms;
}

bool watch_runtime_init(uint32_t now_ms)
{
    s_runtime = (watch_runtime_state_t) { 0 };
    atomic_flag_clear_explicit(&s_ui_event_lock, memory_order_release);
    s_runtime.initialized = true;
    s_runtime.initialized_at_ms = now_ms;
    s_runtime.init_stage = WATCH_RUNTIME_INIT_RESET;
    return true;
}

bool watch_runtime_advance_init(watch_runtime_init_stage_t next_stage)
{
    if (!s_runtime.initialized || next_stage <= WATCH_RUNTIME_INIT_RESET
        || next_stage >= WATCH_RUNTIME_INIT_COUNT || next_stage == WATCH_RUNTIME_INIT_FAILED) {
        return false;
    }

    if ((uint8_t)next_stage != ((uint8_t)s_runtime.init_stage + 1U)) {
        return false;
    }

    s_runtime.init_stage = next_stage;
    return true;
}

void watch_runtime_fail(void)
{
    if (s_runtime.initialized) {
        s_runtime.init_stage = WATCH_RUNTIME_INIT_FAILED;
    }
}

watch_runtime_init_stage_t watch_runtime_init_stage(void)
{
    return s_runtime.initialized ? s_runtime.init_stage : WATCH_RUNTIME_INIT_FAILED;
}

bool watch_runtime_is_ready(void)
{
    return s_runtime.initialized && s_runtime.init_stage == WATCH_RUNTIME_INIT_RUNNING;
}

bool watch_runtime_start_service(watch_runtime_service_t service, uint32_t now_ms)
{
    watch_runtime_health_entry_t *entry;

    if (!s_runtime.initialized || !watch_runtime_valid_service(service)) {
        return false;
    }

    entry = &s_runtime.health[service];
    if (!entry->started) {
        entry->started = true;
        entry->started_at_ms = now_ms;
        entry->last_heartbeat_ms = now_ms;
        entry->heartbeat_count = 0U;
    }

    return true;
}

bool watch_runtime_heartbeat(watch_runtime_service_t service, uint32_t now_ms)
{
    watch_runtime_health_entry_t *entry;

    if (!s_runtime.initialized || !watch_runtime_valid_service(service)) {
        return false;
    }

    entry = &s_runtime.health[service];
    if (!entry->started) {
        return false;
    }

    entry->last_heartbeat_ms = now_ms;
    entry->heartbeat_count++;
    return true;
}

bool watch_runtime_read_health(watch_runtime_service_t service, uint32_t now_ms,
                               watch_runtime_health_t *health)
{
    const watch_runtime_health_entry_t *entry;

    if (!s_runtime.initialized || !watch_runtime_valid_service(service) || health == NULL) {
        return false;
    }

    entry = &s_runtime.health[service];
    health->started_at_ms = entry->started_at_ms;
    health->last_heartbeat_ms = entry->last_heartbeat_ms;
    health->heartbeat_count = entry->heartbeat_count;
    if (!entry->started) {
        health->state = WATCH_RUNTIME_HEALTH_NOT_STARTED;
    } else if (watch_runtime_elapsed_ms(now_ms, entry->last_heartbeat_ms)
               >= WATCH_RUNTIME_HEARTBEAT_TIMEOUT_MS) {
        health->state = WATCH_RUNTIME_HEALTH_STALE;
    } else {
        health->state = WATCH_RUNTIME_HEALTH_HEALTHY;
    }

    return true;
}

bool watch_runtime_post_ui_event(const watch_ui_event_t *event)
{
    bool posted = false;

    if (!s_runtime.initialized || event == NULL) {
        return false;
    }

    watch_runtime_lock_ui_events();
    if (s_runtime.ui_event_count < WATCH_RUNTIME_UI_EVENT_QUEUE_CAPACITY) {
        s_runtime.ui_events[s_runtime.ui_event_head] = *event;
        s_runtime.ui_event_head =
            (uint8_t)((s_runtime.ui_event_head + 1U) % WATCH_RUNTIME_UI_EVENT_QUEUE_CAPACITY);
        s_runtime.ui_event_count++;
        posted = true;
    }

    watch_runtime_unlock_ui_events();
    return posted;
}

bool watch_runtime_take_ui_event(watch_ui_event_t *event)
{
    bool taken = false;

    if (!s_runtime.initialized || event == NULL) {
        return false;
    }

    watch_runtime_lock_ui_events();
    if (s_runtime.ui_event_count > 0U) {
        *event = s_runtime.ui_events[s_runtime.ui_event_tail];
        s_runtime.ui_event_tail =
            (uint8_t)((s_runtime.ui_event_tail + 1U) % WATCH_RUNTIME_UI_EVENT_QUEUE_CAPACITY);
        s_runtime.ui_event_count--;
        taken = true;
    }

    watch_runtime_unlock_ui_events();
    return taken;
}

uint8_t watch_runtime_ui_event_count(void)
{
    uint8_t count;

    if (!s_runtime.initialized) {
        return 0U;
    }

    watch_runtime_lock_ui_events();
    count = s_runtime.ui_event_count;
    watch_runtime_unlock_ui_events();
    return count;
}
