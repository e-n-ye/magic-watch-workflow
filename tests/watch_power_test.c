#include "watch_power_state.h"
#include "watch_watchdog.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint32_t refresh_count;
    bool fail;
} fake_watchdog_t;

static bool fake_refresh(void *context)
{
    fake_watchdog_t *watchdog = (fake_watchdog_t *)context;

    watchdog->refresh_count++;
    return !watchdog->fail;
}

static void test_power_state_transitions(void)
{
    watch_power_state_t power;
    watch_power_snapshot_t snapshot;

    assert(watch_power_state_init(&power));
    assert(watch_power_state_dispatch(&power, WATCH_POWER_EVENT_DISPLAY_TIMEOUT));
    assert(watch_power_state_dispatch(&power, WATCH_POWER_EVENT_STOP_REQUEST));
    assert(watch_power_state_dispatch(&power, WATCH_POWER_EVENT_WAKE_RTC));
    assert(watch_power_state_dispatch(&power, WATCH_POWER_EVENT_SOFTWARE_OFF));
    assert(watch_power_state_dispatch(&power, WATCH_POWER_EVENT_WAKE_KEY));
    assert(!watch_power_state_dispatch(&power, WATCH_POWER_EVENT_COUNT));
    assert(watch_power_state_read(&power, &snapshot));
    assert(snapshot.state == WATCH_POWER_STATE_ACTIVE);
    assert(snapshot.wake_source == WATCH_POWER_WAKE_KEY);
    assert(snapshot.transition_count == 5U);
}

static void test_watchdog_requires_health_and_refreshes_periodically(void)
{
    fake_watchdog_t fake = { 0 };
    watch_watchdog_t watchdog;
    watch_watchdog_status_t status;

    assert(watch_watchdog_init(&watchdog, fake_refresh, &fake, 0U));
    assert(watch_watchdog_process(&watchdog, 0U, true));
    assert(fake.refresh_count == 1U);
    assert(watch_watchdog_process(&watchdog, 50U, true));
    assert(fake.refresh_count == 1U);
    assert(watch_watchdog_process(&watchdog, 100U, true));
    assert(fake.refresh_count == 2U);
    assert(!watch_watchdog_process(&watchdog, 200U, false));

    fake.fail = true;
    assert(!watch_watchdog_process(&watchdog, 300U, true));
    assert(watch_watchdog_read_status(&watchdog, &status));
    assert(status.enabled);
    assert(status.refresh_count == 2U);
    assert(status.blocked_count == 1U);
    assert(status.refresh_failure_count == 1U);
}

int main(void)
{
    test_power_state_transitions();
    test_watchdog_requires_health_and_refreshes_periodically();
    return 0;
}
