#ifndef WATCH_OTA_BOARD_H
#define WATCH_OTA_BOARD_H

#include <stdbool.h>
#include <stdint.h>

#include "watch_ota_metadata.h"
#include "watch_ota_package.h"
#include "watch_ota_trial.h"
#include "watch_ymodem.h"

typedef enum {
    WATCH_OTA_DOWNLOAD_USB = 0,
    WATCH_OTA_DOWNLOAD_BLUETOOTH,
    WATCH_OTA_DOWNLOAD_CHANNEL_COUNT
} watch_ota_download_channel_t;

typedef size_t (*watch_ota_download_read_fn)(void *context, uint8_t *data, size_t length);
typedef size_t (*watch_ota_download_write_fn)(void *context, const uint8_t *data, size_t length);

typedef struct
{
    bool initialized;
    uint32_t jedec_id;
    watch_w25q128_result_t flash_result;
    watch_ota_metadata_result_t metadata_result;
    bool record_valid;
    watch_ota_metadata_record_t record;
    bool download_active;
    watch_ota_download_channel_t download_channel;
    watch_ymodem_state_t download_state;
    watch_ymodem_result_t download_result;
} watch_ota_board_status_t;

bool watch_ota_board_init(void);
bool watch_ota_board_read_status(watch_ota_board_status_t *status);
watch_ota_package_result_t watch_ota_board_verify_candidate(watch_ota_package_info_t *info);
watch_ota_metadata_result_t watch_ota_board_stage_candidate(void);
watch_ota_metadata_result_t watch_ota_board_confirm_trial(void);
watch_ota_metadata_result_t watch_ota_board_mark_trial_fault(uint32_t error_code);
bool watch_ota_board_reset_metadata(void);
bool watch_ota_board_start_download(watch_ota_download_channel_t channel,
                                    watch_ota_download_read_fn read,
                                    watch_ota_download_write_fn write, void *context);
size_t watch_ota_board_process_download(void);
bool watch_ota_board_download_active(void);

#endif /* WATCH_OTA_BOARD_H */
