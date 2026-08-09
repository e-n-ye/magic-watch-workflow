#include "watch_runtime.h"

#include <assert.h>
#include <stdint.h>

static void test_elapsed_wraps(void)
{
    assert(watch_runtime_elapsed_ms(4U, UINT32_MAX - 5U) == 10U);
    assert(watch_runtime_elapsed_ms(200U, 100U) == 100U);
}

static void test_initialization_policy(void)
{
    assert(!watch_runtime_advance_init(WATCH_RUNTIME_INIT_CORE));
    assert(watch_runtime_init(0U));
    assert(watch_runtime_init_stage() == WATCH_RUNTIME_INIT_RESET);
    assert(!watch_runtime_is_ready());
    assert(!watch_runtime_advance_init(WATCH_RUNTIME_INIT_INPUT));
    assert(watch_runtime_advance_init(WATCH_RUNTIME_INIT_CORE));
    assert(watch_runtime_advance_init(WATCH_RUNTIME_INIT_INPUT));
    assert(watch_runtime_advance_init(WATCH_RUNTIME_INIT_RUNNING));
    assert(watch_runtime_is_ready());

    watch_runtime_fail();
    assert(watch_runtime_init_stage() == WATCH_RUNTIME_INIT_FAILED);
    assert(!watch_runtime_is_ready());
}

static void test_service_queue_is_bounded_and_ordered(void)
{
    watch_service_event_t event;

    assert(watch_runtime_init(0U));
    assert(watch_runtime_service_event_count() == 0U);
    for (uint16_t index = 0U; index < WATCH_RUNTIME_SERVICE_QUEUE_CAPACITY; index++) {
        event = (watch_service_event_t) {
            .type = 1U,
            .value = index,
            .timestamp_ms = index * 10U,
        };
        assert(watch_runtime_post_service_event(&event));
    }
    assert(watch_runtime_service_event_count() == WATCH_RUNTIME_SERVICE_QUEUE_CAPACITY);
    assert(!watch_runtime_post_service_event(&event));

    for (uint16_t index = 0U; index < WATCH_RUNTIME_SERVICE_QUEUE_CAPACITY; index++) {
        assert(watch_runtime_take_service_event(&event));
        assert(event.value == index);
        assert(event.timestamp_ms == index * 10U);
    }
    assert(watch_runtime_service_event_count() == 0U);
    assert(!watch_runtime_take_service_event(&event));
}

static void test_service_health(void)
{
    watch_runtime_health_t health;
    const uint32_t start_ms = UINT32_MAX - 1000U;

    assert(watch_runtime_init(start_ms));
    assert(watch_runtime_read_health(WATCH_RUNTIME_SERVICE_UI, start_ms, &health));
    assert(health.state == WATCH_RUNTIME_HEALTH_NOT_STARTED);
    assert(!watch_runtime_heartbeat(WATCH_RUNTIME_SERVICE_UI, start_ms));

    assert(watch_runtime_start_service(WATCH_RUNTIME_SERVICE_APP, start_ms));
    assert(watch_runtime_start_service(WATCH_RUNTIME_SERVICE_APP, start_ms + 1U));
    assert(watch_runtime_heartbeat(WATCH_RUNTIME_SERVICE_APP, start_ms + 2U));
    assert(watch_runtime_read_health(WATCH_RUNTIME_SERVICE_APP, start_ms + 2U, &health));
    assert(health.state == WATCH_RUNTIME_HEALTH_HEALTHY);
    assert(health.heartbeat_count == 1U);
    assert(health.started_at_ms == start_ms);

    assert(watch_runtime_read_health(WATCH_RUNTIME_SERVICE_APP,
                                     start_ms + 2U + WATCH_RUNTIME_HEARTBEAT_TIMEOUT_MS,
                                     &health));
    assert(health.state == WATCH_RUNTIME_HEALTH_STALE);
}

int main(void)
{
    test_elapsed_wraps();
    test_initialization_policy();
    test_service_queue_is_bounded_and_ordered();
    test_service_health();
    return 0;
}
