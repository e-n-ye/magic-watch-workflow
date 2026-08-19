#ifndef WATCH_OTA_INSTALL_H
#define WATCH_OTA_INSTALL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "watch_ota_metadata.h"

#define WATCH_OTA_INSTALL_BLOCK_SIZE 256U
#define WATCH_OTA_INSTALL_SECTOR_SIZE 4096U

typedef enum {
    WATCH_OTA_INSTALL_REGION_APP = 0,
    WATCH_OTA_INSTALL_REGION_ROLLBACK,
    WATCH_OTA_INSTALL_REGION_CANDIDATE,
    WATCH_OTA_INSTALL_REGION_COUNT
} watch_ota_install_region_t;

typedef enum {
    WATCH_OTA_INSTALL_RESULT_OK = 0,
    WATCH_OTA_INSTALL_RESULT_COMPLETE,
    WATCH_OTA_INSTALL_RESULT_INVALID_ARGUMENT,
    WATCH_OTA_INSTALL_RESULT_INVALID_STATE,
    WATCH_OTA_INSTALL_RESULT_RANGE,
    WATCH_OTA_INSTALL_RESULT_IO,
    WATCH_OTA_INSTALL_RESULT_VERIFY,
    WATCH_OTA_INSTALL_RESULT_PERSIST,
    WATCH_OTA_INSTALL_RESULT_COUNT
} watch_ota_install_result_t;

typedef bool (*watch_ota_install_read_fn)(void *context, watch_ota_install_region_t region,
                                          uint32_t offset, uint8_t *data, size_t length);
typedef bool (*watch_ota_install_erase_fn)(void *context, watch_ota_install_region_t region,
                                           uint32_t offset, size_t length);
typedef bool (*watch_ota_install_write_fn)(void *context, watch_ota_install_region_t region,
                                           uint32_t offset, const uint8_t *data, size_t length);
typedef bool (*watch_ota_install_persist_fn)(void *context,
                                             const watch_ota_metadata_record_t *record);

void watch_ota_install_progress(void);

typedef struct
{
    watch_ota_install_read_fn read;
    watch_ota_install_erase_fn erase;
    watch_ota_install_write_fn write;
    watch_ota_install_persist_fn persist;
    void *context;
    uint32_t app_size;
    uint32_t rollback_size;
    uint32_t candidate_size;
    uint32_t sector_size;
} watch_ota_install_config_t;

typedef struct
{
    watch_ota_install_config_t config;
    bool initialized;
} watch_ota_install_t;

bool watch_ota_install_init(watch_ota_install_t *install, const watch_ota_install_config_t *config);
watch_ota_install_result_t watch_ota_install_start(watch_ota_install_t *install,
                                                   watch_ota_metadata_record_t *record);
watch_ota_install_result_t watch_ota_install_step(watch_ota_install_t *install,
                                                  watch_ota_metadata_record_t *record);
const char *watch_ota_install_result_name(watch_ota_install_result_t result);

#endif /* WATCH_OTA_INSTALL_H */
