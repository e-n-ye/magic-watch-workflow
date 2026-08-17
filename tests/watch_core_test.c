#include "watch_core.h"

#include <assert.h>
#include <stddef.h>

#include "watch_input.h"

static watch_event_t watch_test_event(watch_event_type_t type)
{
    return (watch_event_t){.type = type};
}

static bool watch_test_dispatch(watch_core_t *core, watch_event_type_t type)
{
    watch_event_t event = watch_test_event(type);

    return watch_core_dispatch_event(core, &event);
}

static bool watch_test_dispatch_time(watch_core_t *core, watch_time_value_t time)
{
    watch_event_t event = {
        .type = WATCH_EVENT_TIME_UPDATED,
        .time = time,
    };

    return watch_core_dispatch_event(core, &event);
}

static void watch_test_expect_input_event(watch_input_t *input, watch_event_type_t type)
{
    watch_event_t event;

    assert(watch_input_take_event(input, &event));
    assert(event.type == type);
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
    assert(!snapshot.popup_visible);
    assert(snapshot.sensor_snapshot.degraded);
    assert(snapshot.sensor_snapshot.available_mask == 0U);
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
    assert(!command.popup_visible);
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

static void test_resource_navigation(void)
{
    watch_core_t core;
    watch_snapshot_t snapshot;

    assert(watch_core_init(&core));
    assert(watch_test_dispatch(&core, WATCH_EVENT_SELECT));
    assert(watch_test_dispatch(&core, WATCH_EVENT_DOWN));
    assert(watch_test_dispatch(&core, WATCH_EVENT_DOWN));
    assert(watch_core_read_snapshot(&core, &snapshot));
    assert(snapshot.page == WATCH_PAGE_LAUNCHER);
    assert(snapshot.launcher_index == 2U);

    assert(watch_test_dispatch(&core, WATCH_EVENT_SELECT));
    assert(watch_core_read_snapshot(&core, &snapshot));
    assert(snapshot.page == WATCH_PAGE_RESOURCES);
    assert(snapshot.page_depth == 2U);

    assert(watch_test_dispatch(&core, WATCH_EVENT_BACK));
    assert(watch_test_dispatch(&core, WATCH_EVENT_BACK));
    assert(watch_core_read_snapshot(&core, &snapshot));
    assert(snapshot.page == WATCH_PAGE_WATCHFACE);
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

static void test_time_event_updates_snapshot_and_command(void)
{
    watch_core_t core;
    watch_command_t command;
    watch_snapshot_t snapshot;
    watch_time_value_t time = {
        .year = 2026U,
        .month = 8U,
        .day = 16U,
        .weekday = WATCH_TIME_WEEKDAY_SUNDAY,
        .hour = 9U,
        .minute = 30U,
        .second = 45U,
    };

    assert(watch_core_init(&core));
    assert(watch_test_dispatch_time(&core, time));
    assert(watch_core_read_snapshot(&core, &snapshot));
    assert(snapshot.time_valid);
    assert(watch_time_equal(&snapshot.time, &time));
    assert(snapshot.revision == 1U);
    assert(watch_core_take_command(&core, &command));
    assert(command.type == WATCH_COMMAND_TIME_CHANGED);
    assert(command.time_valid);
    assert(watch_time_equal(&command.time, &time));
    assert(watch_test_dispatch_time(&core, time));
    assert(!watch_core_take_command(&core, &command));
    assert(watch_core_read_snapshot(&core, &snapshot));
    assert(snapshot.revision == 1U);
    time.year = 1999U;
    assert(!watch_test_dispatch_time(&core, time));
}

static void test_sensor_status_event_updates_snapshot_and_command(void)
{
    watch_core_t core;
    watch_command_t command;
    watch_sensor_aggregate_service_t aggregate;
    watch_sensor_aggregate_status_t statuses[WATCH_SENSOR_AGGREGATE_SENSOR_COUNT] = { 0 };
    watch_sensor_aggregate_snapshot_t sensor_snapshot;
    watch_snapshot_t snapshot;
    watch_event_t event;

    statuses[WATCH_SENSOR_AGGREGATE_LSM6DS3].ready = true;
    statuses[WATCH_SENSOR_AGGREGATE_LSM6DS3].sample_valid = true;
    statuses[WATCH_SENSOR_AGGREGATE_LSM6DS3].state = 1U;
    statuses[WATCH_SENSOR_AGGREGATE_LSM6DS3].sample_count = 10U;
    assert(watch_sensor_aggregate_init(&aggregate));
    assert(watch_sensor_aggregate_update(&aggregate, statuses, &sensor_snapshot));
    assert(watch_core_init(&core));

    event = (watch_event_t) {
        .type = WATCH_EVENT_SENSOR_STATUS_UPDATED,
        .sensor_snapshot = sensor_snapshot,
    };
    assert(watch_core_dispatch_event(&core, &event));
    assert(watch_core_read_snapshot(&core, &snapshot));
    assert(snapshot.sensor_snapshot.degraded);
    assert(snapshot.sensor_snapshot.available_mask == 1U);
    assert(snapshot.sensor_snapshot.revision == 1U);
    assert(watch_core_take_command(&core, &command));
    assert(command.type == WATCH_COMMAND_SENSOR_STATUS_CHANGED);
    assert(command.sensor_snapshot.available_mask == 1U);
    assert(watch_core_dispatch_event(&core, &event));
    assert(!watch_core_take_command(&core, &command));
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

static void test_button_debounce(void)
{
    watch_input_t input;

    assert(watch_input_init(&input));
    assert(watch_input_seed_button(&input, WATCH_INPUT_BUTTON_WAKE, true, 0U));
    assert(watch_input_submit_button(&input, WATCH_INPUT_BUTTON_WAKE, true, 100U));
    assert(!watch_input_take_event(&input, &(watch_event_t){0}));

    assert(watch_input_submit_button(&input, WATCH_INPUT_BUTTON_BACK, false, 0U));
    assert(watch_input_submit_button(&input, WATCH_INPUT_BUTTON_BACK, true, 10U));
    assert(!watch_input_take_event(&input, &(watch_event_t){0}));
    assert(watch_input_submit_button(&input, WATCH_INPUT_BUTTON_BACK, true, 39U));
    assert(!watch_input_take_event(&input, &(watch_event_t){0}));
    assert(watch_input_submit_button(&input, WATCH_INPUT_BUTTON_BACK, true, 40U));
    watch_test_expect_input_event(&input, WATCH_EVENT_BACK);

    assert(watch_input_submit_button(&input, WATCH_INPUT_BUTTON_BACK, false, 41U));
    assert(watch_input_submit_button(&input, WATCH_INPUT_BUTTON_BACK, false, 71U));
    assert(watch_input_submit_button(&input, WATCH_INPUT_BUTTON_BACK, true, 100U));
    assert(watch_input_submit_button(&input, WATCH_INPUT_BUTTON_BACK, true, 130U));
    watch_test_expect_input_event(&input, WATCH_EVENT_BACK);
}

static void test_encoder_and_touch_mapping(void)
{
    watch_input_t input;
    watch_core_t core;
    watch_event_t event;
    watch_input_touch_t touch;

    assert(watch_input_init(&input));
    assert(watch_core_init(&core));

    assert(watch_input_submit_button(&input, WATCH_INPUT_BUTTON_ENCODER, true, 0U));
    assert(watch_input_submit_button(&input, WATCH_INPUT_BUTTON_ENCODER, true, 30U));
    assert(watch_input_take_event(&input, &event));
    assert(event.type == WATCH_EVENT_SELECT);
    assert(watch_core_dispatch_event(&core, &event));

    touch = (watch_input_touch_t){.start_x = 100U, .start_y = 100U, .end_x = 102U, .end_y = 101U};
    assert(watch_input_submit_touch(&input, &touch));
    watch_test_expect_input_event(&input, WATCH_EVENT_SELECT);

    assert(watch_input_submit_encoder(&input, 1));
    assert(watch_input_take_event(&input, &event));
    assert(event.type == WATCH_EVENT_DOWN);
    assert(watch_core_dispatch_event(&core, &event));

    touch = (watch_input_touch_t){.start_x = 4U, .start_y = 100U, .end_x = 60U, .end_y = 102U};
    assert(watch_input_submit_touch(&input, &touch));
    assert(watch_input_take_event(&input, &event));
    assert(event.type == WATCH_EVENT_BACK);
    assert(watch_core_dispatch_event(&core, &event));

    touch = (watch_input_touch_t){.start_x = 100U, .start_y = 180U, .end_x = 102U, .end_y = 120U};
    assert(watch_input_submit_touch(&input, &touch));
    watch_test_expect_input_event(&input, WATCH_EVENT_UP);

    touch = (watch_input_touch_t){.start_x = 100U, .start_y = 120U, .end_x = 102U, .end_y = 180U};
    assert(watch_input_submit_touch(&input, &touch));
    watch_test_expect_input_event(&input, WATCH_EVENT_DOWN);

    touch = (watch_input_touch_t){.start_x = 120U, .start_y = 100U, .end_x = 180U, .end_y = 102U};
    assert(watch_input_submit_touch(&input, &touch));
    assert(!watch_input_take_event(&input, &event));
}

static void test_input_queue_is_bounded(void)
{
    watch_input_t input;
    watch_input_t button_input;
    watch_event_t event;

    assert(watch_input_init(&input));
    for (uint8_t index = 0U; index < WATCH_INPUT_EVENT_CAPACITY; index++) {
        assert(watch_input_submit_encoder(&input, 1));
    }
    assert(!watch_input_submit_encoder(&input, 1));
    watch_test_expect_input_event(&input, WATCH_EVENT_DOWN);
    assert(watch_input_submit_encoder(&input, -1));

    assert(watch_input_init(&button_input));
    for (uint8_t index = 0U; index < WATCH_INPUT_EVENT_CAPACITY; index++) {
        assert(watch_input_submit_encoder(&button_input, 1));
    }
    assert(watch_input_submit_button(&button_input, WATCH_INPUT_BUTTON_BACK, true, 0U));
    assert(!watch_input_submit_button(&button_input, WATCH_INPUT_BUTTON_BACK, true, 30U));
    for (uint8_t index = 0U; index < WATCH_INPUT_EVENT_CAPACITY; index++) {
        assert(watch_input_take_event(&button_input, &event));
    }
    assert(watch_input_submit_button(&button_input, WATCH_INPUT_BUTTON_BACK, true, 30U));
    assert(watch_input_take_event(&button_input, &event));
    assert(event.type == WATCH_EVENT_BACK);
}

int main(void)
{
    test_initial_state();
    test_navigation_and_commands();
    test_resource_navigation();
    test_ignored_events_and_validation();
    test_time_event_updates_snapshot_and_command();
    test_sensor_status_event_updates_snapshot_and_command();
    test_command_queue_is_bounded();
    test_button_debounce();
    test_encoder_and_touch_mapping();
    test_input_queue_is_bounded();
    return 0;
}
