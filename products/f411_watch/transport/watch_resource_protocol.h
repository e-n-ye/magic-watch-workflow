#ifndef WATCH_RESOURCE_PROTOCOL_H
#define WATCH_RESOURCE_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <tinycrypt/sha256.h>

#define WATCH_RESOURCE_PROTOCOL_VERSION 1U
#define WATCH_RESOURCE_PROTOCOL_HEADER_SIZE 18U
#define WATCH_RESOURCE_PROTOCOL_MAX_PAYLOAD 512U
#define WATCH_RESOURCE_PROTOCOL_MAX_FRAME_SIZE                                                     \
    (WATCH_RESOURCE_PROTOCOL_HEADER_SIZE + WATCH_RESOURCE_PROTOCOL_MAX_PAYLOAD)
#define WATCH_RESOURCE_PROTOCOL_MAX_PATH 128U
#define WATCH_RESOURCE_PROTOCOL_DIGEST_SIZE 32U
#define WATCH_RESOURCE_PROTOCOL_MAX_FILE_SIZE 0x00EF0000UL

typedef enum {
    WATCH_RESOURCE_FRAME_BEGIN = 1U,
    WATCH_RESOURCE_FRAME_DATA = 2U,
    WATCH_RESOURCE_FRAME_COMMIT = 3U,
    WATCH_RESOURCE_FRAME_ABORT = 4U,
    WATCH_RESOURCE_FRAME_ACK = 0x80U,
    WATCH_RESOURCE_FRAME_NACK = 0x81U,
} watch_resource_frame_type_t;

typedef enum {
    WATCH_RESOURCE_ERROR_NONE = 0U,
    WATCH_RESOURCE_ERROR_VERSION,
    WATCH_RESOURCE_ERROR_FLAGS,
    WATCH_RESOURCE_ERROR_LENGTH,
    WATCH_RESOURCE_ERROR_CRC,
    WATCH_RESOURCE_ERROR_TYPE,
    WATCH_RESOURCE_ERROR_SEQUENCE,
    WATCH_RESOURCE_ERROR_STATE,
    WATCH_RESOURCE_ERROR_BEGIN,
    WATCH_RESOURCE_ERROR_DATA,
    WATCH_RESOURCE_ERROR_OFFSET,
    WATCH_RESOURCE_ERROR_SIZE,
    WATCH_RESOURCE_ERROR_DIGEST,
    WATCH_RESOURCE_ERROR_STORAGE,
    WATCH_RESOURCE_ERROR_COUNT,
} watch_resource_error_t;

typedef bool (*watch_resource_begin_fn)(void *context, const char *path, uint32_t size,
                                        const uint8_t digest[WATCH_RESOURCE_PROTOCOL_DIGEST_SIZE]);
typedef bool (*watch_resource_data_fn)(void *context, uint32_t offset, const uint8_t *data,
                                       size_t length);
typedef bool (*watch_resource_commit_fn)(void *context);
typedef void (*watch_resource_abort_fn)(void *context);
typedef bool (*watch_resource_emit_fn)(void *context, const uint8_t *data, size_t length);

typedef struct
{
    watch_resource_begin_fn begin;
    watch_resource_data_fn data;
    watch_resource_commit_fn commit;
    watch_resource_abort_fn abort;
    void *context;
} watch_resource_sink_t;

typedef struct
{
    watch_resource_sink_t sink;
    watch_resource_emit_fn emit;
    void *emit_context;
    uint8_t frame[WATCH_RESOURCE_PROTOCOL_MAX_FRAME_SIZE];
    size_t frame_length;
    size_t expected_frame_length;
    bool transfer_active;
    uint32_t expected_sequence;
    uint32_t total_size;
    uint32_t received_size;
    char path[WATCH_RESOURCE_PROTOCOL_MAX_PATH + 1U];
    uint8_t expected_digest[WATCH_RESOURCE_PROTOCOL_DIGEST_SIZE];
    struct tc_sha256_state_struct digest_state;
    uint32_t frame_error_count;
    uint32_t accepted_frame_count;
    uint32_t emit_failure_count;
} watch_resource_protocol_t;

void watch_resource_protocol_init(watch_resource_protocol_t *protocol,
                                  const watch_resource_sink_t *sink, watch_resource_emit_fn emit,
                                  void *emit_context);
void watch_resource_protocol_reset(watch_resource_protocol_t *protocol);
size_t watch_resource_protocol_feed(watch_resource_protocol_t *protocol, const uint8_t *data,
                                    size_t length);
bool watch_resource_protocol_is_active(const watch_resource_protocol_t *protocol);
uint32_t watch_resource_protocol_frame_errors(const watch_resource_protocol_t *protocol);
uint32_t watch_resource_protocol_accepted_frames(const watch_resource_protocol_t *protocol);
uint32_t watch_resource_protocol_emit_failures(const watch_resource_protocol_t *protocol);

#endif /* WATCH_RESOURCE_PROTOCOL_H */
