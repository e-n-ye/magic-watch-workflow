#ifndef F411_BOOTLOADER_STORAGE_H
#define F411_BOOTLOADER_STORAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "watch_ota_install.h"
#include "watch_ota_package.h"
#include "watch_w25_partitions.h"
#include "watch_w25q128.h"

#define F411_BOOTLOADER_APP_BASE 0x08010000UL
#define F411_BOOTLOADER_APP_SIZE WATCH_OTA_PACKAGE_SIZE
#define F411_BOOTLOADER_ROLLBACK_BASE WATCH_W25_ROLLBACK_OFFSET
#define F411_BOOTLOADER_CANDIDATE_BASE WATCH_W25_CANDIDATE_OFFSET

bool f411_bootloader_storage_init(void);
void f411_bootloader_watchdog_extend(void);
void watch_ota_package_progress(void);
void watch_ota_install_progress(void);
bool f411_bootloader_storage_read(void *context, watch_ota_install_region_t region, uint32_t offset,
                                  uint8_t *data, size_t length);
bool f411_bootloader_storage_erase(void *context, watch_ota_install_region_t region,
                                   uint32_t offset, size_t length);
bool f411_bootloader_storage_write(void *context, watch_ota_install_region_t region,
                                   uint32_t offset, const uint8_t *data, size_t length);
bool f411_bootloader_storage_prepare_resume(watch_ota_install_region_t region, uint32_t *progress);
bool f411_bootloader_storage_persist(void *context, const watch_ota_metadata_record_t *record);

bool f411_bootloader_candidate_read(void *context, uint32_t offset, uint8_t *data, size_t length);
watch_w25q128_t *f411_bootloader_w25(void);
watch_ota_metadata_t *f411_bootloader_metadata(void);

#endif /* F411_BOOTLOADER_STORAGE_H */
