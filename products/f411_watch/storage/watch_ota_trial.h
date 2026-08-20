#ifndef WATCH_OTA_TRIAL_H
#define WATCH_OTA_TRIAL_H

#include <stdbool.h>
#include <stdint.h>

#include "watch_ota_metadata.h"

#define WATCH_OTA_TRIAL_CONFIRMATION_MS 30000U
#define WATCH_OTA_TRIAL_MAX_UNCONFIRMED_BOOTS 3U
#define WATCH_OTA_TRIAL_ERROR_BOOT_LIMIT 0x1001U
#define WATCH_OTA_TRIAL_ERROR_HEALTH_TIMEOUT 0x1002U
#define WATCH_OTA_TRIAL_ERROR_DIAGNOSTIC 0x1003U

typedef enum {
    WATCH_OTA_TRIAL_BOOT_JUMP = 0,
    WATCH_OTA_TRIAL_BOOT_ROLLBACK,
    WATCH_OTA_TRIAL_BOOT_INVALID
} watch_ota_trial_boot_result_t;

typedef enum {
    WATCH_OTA_TRIAL_RESULT_OK = 0,
    WATCH_OTA_TRIAL_RESULT_NOT_READY,
    WATCH_OTA_TRIAL_RESULT_INVALID_STATE,
    WATCH_OTA_TRIAL_RESULT_INVALID_ARGUMENT,
    WATCH_OTA_TRIAL_RESULT_COUNT
} watch_ota_trial_result_t;

typedef struct
{
    bool ui_healthy;
    bool input_healthy;
    bool supervisor_healthy;
    bool watchdog_healthy;
    bool metadata_healthy;
} watch_ota_trial_health_t;

watch_ota_trial_boot_result_t watch_ota_trial_prepare_boot(watch_ota_metadata_record_t *record);
watch_ota_trial_result_t watch_ota_trial_health_ready(const watch_ota_trial_health_t *health,
                                                      uint32_t elapsed_ms);
watch_ota_trial_result_t watch_ota_trial_confirm(watch_ota_metadata_record_t *record);
watch_ota_trial_result_t watch_ota_trial_mark_fault(watch_ota_metadata_record_t *record,
                                                    uint32_t error_code);
watch_ota_trial_result_t watch_ota_trial_start_rollback(watch_ota_metadata_record_t *record);
watch_ota_trial_result_t watch_ota_trial_complete_rollback(watch_ota_metadata_record_t *record);
const char *watch_ota_trial_result_name(watch_ota_trial_result_t result);

#endif /* WATCH_OTA_TRIAL_H */
