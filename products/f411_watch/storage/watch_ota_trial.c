#include "watch_ota_trial.h"

#include <string.h>

watch_ota_trial_boot_result_t watch_ota_trial_prepare_boot(watch_ota_metadata_record_t *record)
{
    if (record == NULL || record->state != WATCH_OTA_METADATA_TRIAL) {
        return WATCH_OTA_TRIAL_BOOT_INVALID;
    }

    if (record->trial_count >= WATCH_OTA_TRIAL_MAX_UNCONFIRMED_BOOTS) {
        record->state = WATCH_OTA_METADATA_PENDING_ROLLBACK;
        record->error_code = WATCH_OTA_TRIAL_ERROR_BOOT_LIMIT;
        return WATCH_OTA_TRIAL_BOOT_ROLLBACK;
    }

    record->trial_count++;
    return WATCH_OTA_TRIAL_BOOT_JUMP;
}

watch_ota_trial_result_t watch_ota_trial_health_ready(const watch_ota_trial_health_t *health,
                                                      uint32_t elapsed_ms)
{
    if (health == NULL) {
        return WATCH_OTA_TRIAL_RESULT_INVALID_ARGUMENT;
    }
    if (elapsed_ms < WATCH_OTA_TRIAL_CONFIRMATION_MS || !health->ui_healthy
        || !health->input_healthy || !health->supervisor_healthy || !health->watchdog_healthy
        || !health->metadata_healthy) {
        return WATCH_OTA_TRIAL_RESULT_NOT_READY;
    }
    return WATCH_OTA_TRIAL_RESULT_OK;
}

watch_ota_trial_result_t watch_ota_trial_confirm(watch_ota_metadata_record_t *record)
{
    if (record == NULL) {
        return WATCH_OTA_TRIAL_RESULT_INVALID_ARGUMENT;
    }
    if (record->state != WATCH_OTA_METADATA_TRIAL) {
        return WATCH_OTA_TRIAL_RESULT_INVALID_STATE;
    }

    record->state = WATCH_OTA_METADATA_CONFIRMED;
    record->confirmed_counter = record->candidate_counter;
    record->active_version = record->candidate_version;
    memcpy(record->active_digest, record->candidate_digest, WATCH_OTA_METADATA_DIGEST_SIZE);
    record->trial_count = 0U;
    record->error_code = 0U;
    return WATCH_OTA_TRIAL_RESULT_OK;
}

watch_ota_trial_result_t watch_ota_trial_mark_fault(watch_ota_metadata_record_t *record,
                                                    uint32_t error_code)
{
    if (record == NULL) {
        return WATCH_OTA_TRIAL_RESULT_INVALID_ARGUMENT;
    }
    if (record->state != WATCH_OTA_METADATA_TRIAL) {
        return WATCH_OTA_TRIAL_RESULT_INVALID_STATE;
    }

    record->state = WATCH_OTA_METADATA_PENDING_ROLLBACK;
    record->error_code = error_code;
    return WATCH_OTA_TRIAL_RESULT_OK;
}

watch_ota_trial_result_t watch_ota_trial_start_rollback(watch_ota_metadata_record_t *record)
{
    if (record == NULL) {
        return WATCH_OTA_TRIAL_RESULT_INVALID_ARGUMENT;
    }
    if (record->state == WATCH_OTA_METADATA_ROLLING_BACK) {
        return WATCH_OTA_TRIAL_RESULT_OK;
    }
    if (record->state != WATCH_OTA_METADATA_PENDING_ROLLBACK) {
        return WATCH_OTA_TRIAL_RESULT_INVALID_STATE;
    }

    record->state = WATCH_OTA_METADATA_ROLLING_BACK;
    record->progress = 0U;
    return WATCH_OTA_TRIAL_RESULT_OK;
}

watch_ota_trial_result_t watch_ota_trial_complete_rollback(watch_ota_metadata_record_t *record)
{
    if (record == NULL) {
        return WATCH_OTA_TRIAL_RESULT_INVALID_ARGUMENT;
    }
    if (record->state != WATCH_OTA_METADATA_ROLLING_BACK) {
        return WATCH_OTA_TRIAL_RESULT_INVALID_STATE;
    }

    record->state = WATCH_OTA_METADATA_CONFIRMED;
    record->trial_count = 0U;
    record->error_code = 0U;
    return WATCH_OTA_TRIAL_RESULT_OK;
}

const char *watch_ota_trial_result_name(watch_ota_trial_result_t result)
{
    switch (result) {
    case WATCH_OTA_TRIAL_RESULT_OK:
        return "ok";
    case WATCH_OTA_TRIAL_RESULT_NOT_READY:
        return "not-ready";
    case WATCH_OTA_TRIAL_RESULT_INVALID_STATE:
        return "state";
    case WATCH_OTA_TRIAL_RESULT_INVALID_ARGUMENT:
        return "argument";
    case WATCH_OTA_TRIAL_RESULT_COUNT:
        return "invalid";
    }
    return "invalid";
}
