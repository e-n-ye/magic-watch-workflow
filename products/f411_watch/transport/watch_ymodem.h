#ifndef WATCH_YMODEM_H
#define WATCH_YMODEM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WATCH_YMODEM_SOH 0x01U
#define WATCH_YMODEM_STX 0x02U
#define WATCH_YMODEM_EOT 0x04U
#define WATCH_YMODEM_ACK 0x06U
#define WATCH_YMODEM_NAK 0x15U
#define WATCH_YMODEM_CAN 0x18U
#define WATCH_YMODEM_CRC_REQUEST 'C'
#define WATCH_YMODEM_MAX_FILENAME 64U
#define WATCH_YMODEM_MAX_BLOCK_SIZE 1024U
#define WATCH_YMODEM_MAX_PACKET_SIZE (WATCH_YMODEM_MAX_BLOCK_SIZE + 5U)

typedef enum {
    WATCH_YMODEM_STATE_IDLE = 0,
    WATCH_YMODEM_STATE_HEADER,
    WATCH_YMODEM_STATE_DATA,
    WATCH_YMODEM_STATE_COMPLETE,
    WATCH_YMODEM_STATE_ERROR,
    WATCH_YMODEM_STATE_COUNT
} watch_ymodem_state_t;

typedef enum {
    WATCH_YMODEM_RESULT_OK = 0,
    WATCH_YMODEM_RESULT_COMPLETE,
    WATCH_YMODEM_RESULT_INVALID_ARGUMENT,
    WATCH_YMODEM_RESULT_BUSY,
    WATCH_YMODEM_RESULT_PROTOCOL,
    WATCH_YMODEM_RESULT_CRC,
    WATCH_YMODEM_RESULT_SEQUENCE,
    WATCH_YMODEM_RESULT_SIZE,
    WATCH_YMODEM_RESULT_STORAGE,
    WATCH_YMODEM_RESULT_CANCELLED,
    WATCH_YMODEM_RESULT_IO,
    WATCH_YMODEM_RESULT_COUNT
} watch_ymodem_result_t;

typedef bool (*watch_ymodem_emit_fn)(void *context, uint8_t byte);
typedef bool (*watch_ymodem_begin_fn)(void *context, const char *name, uint32_t size);
typedef bool (*watch_ymodem_data_fn)(void *context, uint32_t offset, const uint8_t *data,
                                     size_t length);
typedef bool (*watch_ymodem_commit_fn)(void *context);
typedef void (*watch_ymodem_abort_fn)(void *context);

typedef struct
{
    watch_ymodem_begin_fn begin;
    watch_ymodem_data_fn data;
    watch_ymodem_commit_fn commit;
    watch_ymodem_abort_fn abort;
    void *context;
} watch_ymodem_sink_t;

typedef struct
{
    watch_ymodem_sink_t sink;
    watch_ymodem_emit_fn emit;
    void *emit_context;
    watch_ymodem_state_t state;
    watch_ymodem_result_t result;
    uint8_t packet[WATCH_YMODEM_MAX_PACKET_SIZE];
    size_t packet_length;
    size_t packet_expected;
    uint16_t block_size;
    uint8_t expected_block;
    uint32_t file_size;
    uint32_t file_offset;
    bool file_started;
    char filename[WATCH_YMODEM_MAX_FILENAME];
} watch_ymodem_t;

bool watch_ymodem_init(watch_ymodem_t *protocol, const watch_ymodem_sink_t *sink,
                       watch_ymodem_emit_fn emit, void *emit_context);
watch_ymodem_result_t watch_ymodem_start(watch_ymodem_t *protocol);
size_t watch_ymodem_feed(watch_ymodem_t *protocol, const uint8_t *data, size_t length);
watch_ymodem_state_t watch_ymodem_state(const watch_ymodem_t *protocol);
watch_ymodem_result_t watch_ymodem_result(const watch_ymodem_t *protocol);
const char *watch_ymodem_state_name(watch_ymodem_state_t state);
const char *watch_ymodem_result_name(watch_ymodem_result_t result);

#endif /* WATCH_YMODEM_H */
