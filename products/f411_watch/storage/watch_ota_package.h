#ifndef WATCH_OTA_PACKAGE_H
#define WATCH_OTA_PACKAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WATCH_OTA_PACKAGE_SIZE 0x00070000UL
#define WATCH_OTA_PACKAGE_TRAILER_OFFSET 0x0006F000UL
#define WATCH_OTA_PACKAGE_TRAILER_SIZE 0x1000U
#define WATCH_OTA_PACKAGE_HEADER_SIZE 128U
#define WATCH_OTA_PACKAGE_SIGNED_SIZE 64U
#define WATCH_OTA_PACKAGE_SIGNATURE_SIZE 64U
#define WATCH_OTA_PACKAGE_DIGEST_SIZE 32U
#define WATCH_OTA_PACKAGE_FORMAT_VERSION 1U
#define WATCH_OTA_PACKAGE_BOARD_ID 0x31313446UL
#define WATCH_OTA_PACKAGE_LOAD_ADDRESS 0x08010000UL
#define WATCH_OTA_PACKAGE_KEY_ID 0U

typedef enum {
    WATCH_OTA_PACKAGE_RESULT_OK = 0,
    WATCH_OTA_PACKAGE_RESULT_INVALID_ARGUMENT,
    WATCH_OTA_PACKAGE_RESULT_IO,
    WATCH_OTA_PACKAGE_RESULT_SIZE,
    WATCH_OTA_PACKAGE_RESULT_FORMAT,
    WATCH_OTA_PACKAGE_RESULT_BOARD,
    WATCH_OTA_PACKAGE_RESULT_RANGE,
    WATCH_OTA_PACKAGE_RESULT_PADDING,
    WATCH_OTA_PACKAGE_RESULT_HASH,
    WATCH_OTA_PACKAGE_RESULT_SIGNATURE,
    WATCH_OTA_PACKAGE_RESULT_SECURITY_REJECTED,
    WATCH_OTA_PACKAGE_RESULT_COUNT
} watch_ota_package_result_t;

typedef bool (*watch_ota_package_read_fn)(void *context, uint32_t offset, uint8_t *data,
                                          size_t length);

typedef struct
{
    watch_ota_package_read_fn read;
    void *context;
    uint32_t size;
} watch_ota_package_reader_t;

typedef struct
{
    uint32_t board_id;
    uint32_t firmware_version;
    uint32_t security_counter;
    uint32_t load_address;
    uint32_t image_length;
    uint8_t digest[WATCH_OTA_PACKAGE_DIGEST_SIZE];
    uint8_t key_id;
} watch_ota_package_info_t;

/* Optional platform hook for long-running readers (e.g. a bare-metal
 * bootloader with an independent watchdog). Host and App builds leave it
 * as a no-op. */
void watch_ota_package_progress(void);

watch_ota_package_result_t
watch_ota_package_verify(const watch_ota_package_reader_t *reader, uint32_t expected_board,
                         uint32_t minimum_security_counter,
                         const uint8_t public_key[WATCH_OTA_PACKAGE_SIGNATURE_SIZE],
                         watch_ota_package_info_t *info);
const char *watch_ota_package_result_name(watch_ota_package_result_t result);

#endif /* WATCH_OTA_PACKAGE_H */
