#ifndef WATCH_OTA_BOARD_H
#define WATCH_OTA_BOARD_H

#include <stdbool.h>
#include <stdint.h>

#include "watch_ota_metadata.h"
#include "watch_ota_package.h"

typedef struct
{
    bool initialized;
    uint32_t jedec_id;
    watch_w25q128_result_t flash_result;
    watch_ota_metadata_result_t metadata_result;
    bool record_valid;
    watch_ota_metadata_record_t record;
} watch_ota_board_status_t;

bool watch_ota_board_init(void);
bool watch_ota_board_read_status(watch_ota_board_status_t *status);
watch_ota_package_result_t watch_ota_board_verify_candidate(watch_ota_package_info_t *info);
watch_ota_metadata_result_t watch_ota_board_stage_candidate(void);

#endif /* WATCH_OTA_BOARD_H */
