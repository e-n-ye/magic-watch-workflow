#ifndef WATCH_OTA_METADATA_H
#define WATCH_OTA_METADATA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "watch_w25_partitions.h"
#include "watch_w25q128.h"

#define WATCH_OTA_METADATA_FORMAT_VERSION 1U
#define WATCH_OTA_METADATA_RECORD_SIZE 128U
#define WATCH_OTA_METADATA_DIGEST_SIZE 32U
#define WATCH_OTA_METADATA_SLOT_COUNT 2U
#define WATCH_OTA_METADATA_SLOT_SIZE WATCH_W25Q128_SECTOR_SIZE
#define WATCH_OTA_METADATA_SLOT_OFFSET(index)                                                      \
    (WATCH_W25_METADATA_OFFSET + ((uint32_t)(index) * WATCH_OTA_METADATA_SLOT_SIZE))

typedef enum {
    WATCH_OTA_METADATA_DOWNLOADING = 1,
    WATCH_OTA_METADATA_CANDIDATE_READY,
    WATCH_OTA_METADATA_BACKING_UP,
    WATCH_OTA_METADATA_INSTALLING,
    WATCH_OTA_METADATA_TRIAL,
    WATCH_OTA_METADATA_CONFIRMED,
    WATCH_OTA_METADATA_PENDING_ROLLBACK,
    WATCH_OTA_METADATA_ROLLING_BACK,
    WATCH_OTA_METADATA_ERROR
} watch_ota_metadata_state_t;

typedef enum {
    WATCH_OTA_METADATA_RESULT_OK = 0,
    WATCH_OTA_METADATA_RESULT_EMPTY,
    WATCH_OTA_METADATA_RESULT_INVALID_ARGUMENT,
    WATCH_OTA_METADATA_RESULT_IO,
    WATCH_OTA_METADATA_RESULT_CORRUPT,
    WATCH_OTA_METADATA_RESULT_INVALID_RECORD,
    WATCH_OTA_METADATA_RESULT_SECURITY_REJECTED,
    WATCH_OTA_METADATA_RESULT_CONFLICT,
    WATCH_OTA_METADATA_RESULT_COUNT
} watch_ota_metadata_result_t;

typedef struct
{
    uint32_t sequence;
    watch_ota_metadata_state_t state;
    uint32_t confirmed_counter;
    uint32_t candidate_counter;
    uint32_t active_version;
    uint32_t candidate_version;
    uint32_t image_length;
    uint32_t progress;
    uint32_t trial_count;
    uint32_t error_code;
    uint8_t active_digest[WATCH_OTA_METADATA_DIGEST_SIZE];
    uint8_t candidate_digest[WATCH_OTA_METADATA_DIGEST_SIZE];
} watch_ota_metadata_record_t;

typedef struct
{
    watch_w25q128_t *flash;
    bool initialized;
} watch_ota_metadata_t;

bool watch_ota_metadata_init(watch_ota_metadata_t *metadata, watch_w25q128_t *flash);
watch_ota_metadata_result_t watch_ota_metadata_load(const watch_ota_metadata_t *metadata,
                                                    watch_ota_metadata_record_t *record);
watch_ota_metadata_result_t watch_ota_metadata_commit(watch_ota_metadata_t *metadata,
                                                      const watch_ota_metadata_record_t *record);
const char *watch_ota_metadata_state_name(watch_ota_metadata_state_t state);
const char *watch_ota_metadata_result_name(watch_ota_metadata_result_t result);

#endif /* WATCH_OTA_METADATA_H */
