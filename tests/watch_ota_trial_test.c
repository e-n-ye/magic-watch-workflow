#include <assert.h>
#include <string.h>

#include "watch_ota_trial.h"

static watch_ota_metadata_record_t trial_record(void)
{
    watch_ota_metadata_record_t record = {
        .state = WATCH_OTA_METADATA_TRIAL,
        .confirmed_counter = 24U,
        .candidate_counter = 25U,
        .active_version = 24U,
        .candidate_version = 25U,
        .image_length = 358708U,
    };

    record.candidate_digest[0] = 0x25U;
    return record;
}

static void test_boot_limit(void)
{
    watch_ota_metadata_record_t record = trial_record();

    assert(watch_ota_trial_prepare_boot(&record) == WATCH_OTA_TRIAL_BOOT_JUMP);
    assert(record.trial_count == 1U);
    assert(watch_ota_trial_prepare_boot(&record) == WATCH_OTA_TRIAL_BOOT_JUMP);
    assert(record.trial_count == 2U);
    assert(watch_ota_trial_prepare_boot(&record) == WATCH_OTA_TRIAL_BOOT_JUMP);
    assert(record.trial_count == 3U);
    assert(watch_ota_trial_prepare_boot(&record) == WATCH_OTA_TRIAL_BOOT_ROLLBACK);
    assert(record.state == WATCH_OTA_METADATA_PENDING_ROLLBACK);
    assert(record.trial_count == 3U);
}

static void test_confirmation_requires_all_services(void)
{
    watch_ota_metadata_record_t record = trial_record();
    watch_ota_trial_health_t health = {
        .ui_healthy = true,
        .input_healthy = true,
        .supervisor_healthy = true,
        .watchdog_healthy = true,
        .metadata_healthy = true,
    };

    assert(watch_ota_trial_health_ready(&health, WATCH_OTA_TRIAL_CONFIRMATION_MS - 1U)
           == WATCH_OTA_TRIAL_RESULT_NOT_READY);
    health.ui_healthy = false;
    assert(watch_ota_trial_health_ready(&health, WATCH_OTA_TRIAL_CONFIRMATION_MS)
           == WATCH_OTA_TRIAL_RESULT_NOT_READY);
    health.ui_healthy = true;
    assert(watch_ota_trial_health_ready(&health, WATCH_OTA_TRIAL_CONFIRMATION_MS)
           == WATCH_OTA_TRIAL_RESULT_OK);
    assert(watch_ota_trial_confirm(&record) == WATCH_OTA_TRIAL_RESULT_OK);
    assert(record.state == WATCH_OTA_METADATA_CONFIRMED);
    assert(record.confirmed_counter == record.candidate_counter);
    assert(record.active_version == record.candidate_version);
    assert(record.active_digest[0] == 0x25U);
}

static void test_fault_and_rollback_are_idempotent(void)
{
    watch_ota_metadata_record_t record = trial_record();

    assert(watch_ota_trial_mark_fault(&record, 0x55U) == WATCH_OTA_TRIAL_RESULT_OK);
    assert(record.state == WATCH_OTA_METADATA_PENDING_ROLLBACK);
    assert(record.error_code == 0x55U);
    assert(watch_ota_trial_start_rollback(&record) == WATCH_OTA_TRIAL_RESULT_OK);
    assert(watch_ota_trial_start_rollback(&record) == WATCH_OTA_TRIAL_RESULT_OK);
    assert(record.state == WATCH_OTA_METADATA_ROLLING_BACK);
    assert(watch_ota_trial_complete_rollback(&record) == WATCH_OTA_TRIAL_RESULT_OK);
    assert(record.state == WATCH_OTA_METADATA_CONFIRMED);
    assert(record.error_code == 0U);
}

int main(void)
{
    test_boot_limit();
    test_confirmation_requires_all_services();
    test_fault_and_rollback_are_idempotent();
    return 0;
}
