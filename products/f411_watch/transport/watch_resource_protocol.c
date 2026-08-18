#include "watch_resource_protocol.h"

#include <string.h>

#include <tinycrypt/constants.h>

#define WATCH_RESOURCE_MAGIC_0 ((uint8_t)'M')
#define WATCH_RESOURCE_MAGIC_1 ((uint8_t)'W')
#define WATCH_RESOURCE_MAGIC_2 ((uint8_t)'R')
#define WATCH_RESOURCE_MAGIC_3 ((uint8_t)'P')
#define WATCH_RESOURCE_BEGIN_FIXED_SIZE (2U + 4U + WATCH_RESOURCE_PROTOCOL_DIGEST_SIZE)
#define WATCH_RESOURCE_DATA_OFFSET_SIZE 4U
#define WATCH_RESOURCE_RESPONSE_SIZE 5U

static uint16_t read_u16(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8U);
}

static uint32_t read_u32(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8U) | ((uint32_t)data[2] << 16U)
        | ((uint32_t)data[3] << 24U);
}

static void write_u16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8U);
}

static void write_u32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8U);
    data[2] = (uint8_t)(value >> 16U);
    data[3] = (uint8_t)(value >> 24U);
}

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t length)
{
    for (size_t index = 0U; index < length; ++index) {
        crc ^= data[index];
        for (uint8_t bit = 0U; bit < 8U; ++bit) {
            crc = (crc & 1U) != 0U ? (crc >> 1U) ^ 0xEDB88320UL : crc >> 1U;
        }
    }
    return crc;
}

static uint32_t frame_crc(const uint8_t *frame, size_t payload_length)
{
    uint32_t crc = crc32_update(0xFFFFFFFFUL, frame, 14U);

    crc = crc32_update(crc, frame + WATCH_RESOURCE_PROTOCOL_HEADER_SIZE, payload_length);
    return crc ^ 0xFFFFFFFFUL;
}

static void reset_frame(watch_resource_protocol_t *protocol)
{
    protocol->frame_length = 0U;
    protocol->expected_frame_length = 0U;
}

static void clear_transfer_state(watch_resource_protocol_t *protocol)
{
    protocol->transfer_active = false;
    protocol->expected_sequence = 0U;
    protocol->total_size = 0U;
    protocol->received_size = 0U;
    memset(protocol->path, 0, sizeof(protocol->path));
    memset(protocol->expected_digest, 0, sizeof(protocol->expected_digest));
}

static void abort_transfer(watch_resource_protocol_t *protocol)
{
    if (protocol->transfer_active && protocol->sink.abort != NULL) {
        protocol->sink.abort(protocol->sink.context);
    }
    clear_transfer_state(protocol);
}

static void emit_response(watch_resource_protocol_t *protocol, uint8_t type, uint32_t sequence,
                          watch_resource_error_t error)
{
    uint8_t response[WATCH_RESOURCE_PROTOCOL_HEADER_SIZE + WATCH_RESOURCE_RESPONSE_SIZE] = { 0 };
    uint8_t *payload = response + WATCH_RESOURCE_PROTOCOL_HEADER_SIZE;
    uint32_t next_sequence = protocol->expected_sequence;

    response[0] = WATCH_RESOURCE_MAGIC_0;
    response[1] = WATCH_RESOURCE_MAGIC_1;
    response[2] = WATCH_RESOURCE_MAGIC_2;
    response[3] = WATCH_RESOURCE_MAGIC_3;
    response[4] = WATCH_RESOURCE_PROTOCOL_VERSION;
    response[5] = type;
    write_u16(response + 6U, 0U);
    write_u32(response + 8U, sequence);
    write_u16(response + 12U, WATCH_RESOURCE_RESPONSE_SIZE);
    payload[0] = (uint8_t)error;
    write_u32(payload + 1U, next_sequence);
    write_u32(response + 14U, frame_crc(response, WATCH_RESOURCE_RESPONSE_SIZE));

    if (protocol->emit == NULL
        || !protocol->emit(protocol->emit_context, response, sizeof(response))) {
        ++protocol->emit_failure_count;
    }
}

static void emit_ack(watch_resource_protocol_t *protocol, uint32_t sequence)
{
    emit_response(protocol, WATCH_RESOURCE_FRAME_ACK, sequence, WATCH_RESOURCE_ERROR_NONE);
}

static void emit_nack(watch_resource_protocol_t *protocol, uint32_t sequence,
                      watch_resource_error_t error)
{
    emit_response(protocol, WATCH_RESOURCE_FRAME_NACK, sequence, error);
}

static bool path_is_safe(const uint8_t *path, size_t length)
{
    size_t segment_start = 1U;

    if (path == NULL || length == 0U || length > WATCH_RESOURCE_PROTOCOL_MAX_PATH || path[0] != '/'
        || path[length - 1U] == '/') {
        return false;
    }

    for (size_t index = 1U; index <= length; ++index) {
        if (index != length && path[index] != '/') {
            continue;
        }

        if (index == segment_start || (index - segment_start == 1U && path[segment_start] == '.')
            || (index - segment_start == 2U && path[segment_start] == '.'
                && path[segment_start + 1U] == '.')) {
            return false;
        }
        segment_start = index + 1U;
    }

    return memchr(path, 0, length) == NULL;
}

static bool sequence_is_expected(const watch_resource_protocol_t *protocol, uint32_t sequence)
{
    return protocol->transfer_active && sequence == protocol->expected_sequence;
}

static void handle_begin(watch_resource_protocol_t *protocol, uint32_t sequence,
                         const uint8_t *payload, size_t length)
{
    uint16_t path_length;
    uint32_t total_size;

    if (protocol->transfer_active || length < WATCH_RESOURCE_BEGIN_FIXED_SIZE) {
        emit_nack(protocol, sequence,
                  protocol->transfer_active ? WATCH_RESOURCE_ERROR_STATE
                                            : WATCH_RESOURCE_ERROR_BEGIN);
        return;
    }

    path_length = read_u16(payload);
    if ((size_t)path_length > length - WATCH_RESOURCE_BEGIN_FIXED_SIZE
        || !path_is_safe(payload + 2U, path_length)
        || length != WATCH_RESOURCE_BEGIN_FIXED_SIZE + path_length) {
        emit_nack(protocol, sequence, WATCH_RESOURCE_ERROR_BEGIN);
        return;
    }

    total_size = read_u32(payload + 2U + path_length);
    if (total_size > WATCH_RESOURCE_PROTOCOL_MAX_FILE_SIZE) {
        emit_nack(protocol, sequence, WATCH_RESOURCE_ERROR_SIZE);
        return;
    }
    if (protocol->sink.begin == NULL
        || tc_sha256_init(&protocol->digest_state) != TC_CRYPTO_SUCCESS) {
        emit_nack(protocol, sequence, WATCH_RESOURCE_ERROR_STORAGE);
        return;
    }

    memcpy(protocol->path, payload + 2U, path_length);
    protocol->path[path_length] = '\0';
    memcpy(protocol->expected_digest, payload + 6U + path_length,
           WATCH_RESOURCE_PROTOCOL_DIGEST_SIZE);
    if (!protocol->sink.begin(protocol->sink.context, protocol->path, total_size,
                              protocol->expected_digest)) {
        memset(protocol->path, 0, sizeof(protocol->path));
        emit_nack(protocol, sequence, WATCH_RESOURCE_ERROR_STORAGE);
        return;
    }

    protocol->transfer_active = true;
    protocol->expected_sequence = sequence + 1U;
    protocol->total_size = total_size;
    protocol->received_size = 0U;
    emit_ack(protocol, sequence);
}

static void handle_data(watch_resource_protocol_t *protocol, uint32_t sequence,
                        const uint8_t *payload, size_t length)
{
    uint32_t offset;
    size_t data_length;

    if (!sequence_is_expected(protocol, sequence)) {
        emit_nack(protocol, sequence,
                  protocol->transfer_active ? WATCH_RESOURCE_ERROR_SEQUENCE
                                            : WATCH_RESOURCE_ERROR_STATE);
        return;
    }
    if (length <= WATCH_RESOURCE_DATA_OFFSET_SIZE) {
        emit_nack(protocol, sequence, WATCH_RESOURCE_ERROR_DATA);
        return;
    }

    offset = read_u32(payload);
    data_length = length - WATCH_RESOURCE_DATA_OFFSET_SIZE;
    if (offset != protocol->received_size || data_length > protocol->total_size - offset) {
        emit_nack(protocol, sequence, WATCH_RESOURCE_ERROR_OFFSET);
        return;
    }
    if (protocol->sink.data == NULL
        || !protocol->sink.data(protocol->sink.context, offset,
                                payload + WATCH_RESOURCE_DATA_OFFSET_SIZE, data_length)
        || tc_sha256_update(&protocol->digest_state, payload + WATCH_RESOURCE_DATA_OFFSET_SIZE,
                            data_length)
            != TC_CRYPTO_SUCCESS) {
        abort_transfer(protocol);
        emit_nack(protocol, sequence, WATCH_RESOURCE_ERROR_STORAGE);
        return;
    }

    protocol->received_size += (uint32_t)data_length;
    protocol->expected_sequence = sequence + 1U;
    emit_ack(protocol, sequence);
}

static void handle_commit(watch_resource_protocol_t *protocol, uint32_t sequence,
                          const uint8_t *payload, size_t length)
{
    uint8_t digest[WATCH_RESOURCE_PROTOCOL_DIGEST_SIZE];

    (void)payload;
    if (!sequence_is_expected(protocol, sequence)) {
        emit_nack(protocol, sequence,
                  protocol->transfer_active ? WATCH_RESOURCE_ERROR_SEQUENCE
                                            : WATCH_RESOURCE_ERROR_STATE);
        return;
    }
    if (length != 0U || protocol->received_size != protocol->total_size
        || tc_sha256_final(digest, &protocol->digest_state) != TC_CRYPTO_SUCCESS) {
        abort_transfer(protocol);
        emit_nack(protocol, sequence, WATCH_RESOURCE_ERROR_SIZE);
        return;
    }
    if (memcmp(digest, protocol->expected_digest, sizeof(digest)) != 0) {
        abort_transfer(protocol);
        emit_nack(protocol, sequence, WATCH_RESOURCE_ERROR_DIGEST);
        return;
    }
    if (protocol->sink.commit == NULL || !protocol->sink.commit(protocol->sink.context)) {
        abort_transfer(protocol);
        emit_nack(protocol, sequence, WATCH_RESOURCE_ERROR_STORAGE);
        return;
    }

    clear_transfer_state(protocol);
    emit_ack(protocol, sequence);
}

static void handle_abort(watch_resource_protocol_t *protocol, uint32_t sequence, size_t length)
{
    if (length != 0U) {
        emit_nack(protocol, sequence, WATCH_RESOURCE_ERROR_LENGTH);
        return;
    }
    if (protocol->transfer_active && sequence != protocol->expected_sequence) {
        emit_nack(protocol, sequence, WATCH_RESOURCE_ERROR_SEQUENCE);
        return;
    }

    abort_transfer(protocol);
    emit_ack(protocol, sequence);
}

static void handle_frame(watch_resource_protocol_t *protocol)
{
    const uint8_t type = protocol->frame[5];
    const uint32_t sequence = read_u32(protocol->frame + 8U);
    const uint16_t payload_length = read_u16(protocol->frame + 12U);
    const uint8_t *payload = protocol->frame + WATCH_RESOURCE_PROTOCOL_HEADER_SIZE;
    const uint32_t expected_crc = read_u32(protocol->frame + 14U);

    if (expected_crc != frame_crc(protocol->frame, payload_length)) {
        ++protocol->frame_error_count;
        emit_nack(protocol, sequence, WATCH_RESOURCE_ERROR_CRC);
        return;
    }

    switch (type) {
    case WATCH_RESOURCE_FRAME_BEGIN:
        handle_begin(protocol, sequence, payload, payload_length);
        break;
    case WATCH_RESOURCE_FRAME_DATA:
        handle_data(protocol, sequence, payload, payload_length);
        break;
    case WATCH_RESOURCE_FRAME_COMMIT:
        handle_commit(protocol, sequence, payload, payload_length);
        break;
    case WATCH_RESOURCE_FRAME_ABORT:
        handle_abort(protocol, sequence, payload_length);
        break;
    default:
        emit_nack(protocol, sequence, WATCH_RESOURCE_ERROR_TYPE);
        break;
    }
    ++protocol->accepted_frame_count;
}

static bool header_is_valid(const watch_resource_protocol_t *protocol)
{
    const uint16_t payload_length = read_u16(protocol->frame + 12U);

    return protocol->frame[0] == WATCH_RESOURCE_MAGIC_0
        && protocol->frame[1] == WATCH_RESOURCE_MAGIC_1
        && protocol->frame[2] == WATCH_RESOURCE_MAGIC_2
        && protocol->frame[3] == WATCH_RESOURCE_MAGIC_3
        && protocol->frame[4] == WATCH_RESOURCE_PROTOCOL_VERSION
        && read_u16(protocol->frame + 6U) == 0U
        && payload_length <= WATCH_RESOURCE_PROTOCOL_MAX_PAYLOAD;
}

static void start_frame(watch_resource_protocol_t *protocol, uint8_t byte)
{
    protocol->frame_length = byte == WATCH_RESOURCE_MAGIC_0 ? 1U : 0U;
    protocol->expected_frame_length = 0U;
    if (protocol->frame_length != 0U) {
        protocol->frame[0] = byte;
    }
}

void watch_resource_protocol_init(watch_resource_protocol_t *protocol,
                                  const watch_resource_sink_t *sink, watch_resource_emit_fn emit,
                                  void *emit_context)
{
    if (protocol == NULL) {
        return;
    }

    memset(protocol, 0, sizeof(*protocol));
    if (sink != NULL) {
        protocol->sink = *sink;
    }
    protocol->emit = emit;
    protocol->emit_context = emit_context;
}

void watch_resource_protocol_reset(watch_resource_protocol_t *protocol)
{
    if (protocol == NULL) {
        return;
    }

    abort_transfer(protocol);
    reset_frame(protocol);
}

size_t watch_resource_protocol_feed(watch_resource_protocol_t *protocol, const uint8_t *data,
                                    size_t length)
{
    if (protocol == NULL || data == NULL) {
        return 0U;
    }

    for (size_t index = 0U; index < length; ++index) {
        const uint8_t byte = data[index];

        if (protocol->frame_length == 0U) {
            start_frame(protocol, byte);
            continue;
        }
        if (protocol->frame_length < 4U) {
            const uint8_t magic[] = { WATCH_RESOURCE_MAGIC_0, WATCH_RESOURCE_MAGIC_1,
                                      WATCH_RESOURCE_MAGIC_2, WATCH_RESOURCE_MAGIC_3 };
            if (byte != magic[protocol->frame_length]) {
                ++protocol->frame_error_count;
                start_frame(protocol, byte);
                continue;
            }
        }

        if (protocol->frame_length >= sizeof(protocol->frame)) {
            ++protocol->frame_error_count;
            reset_frame(protocol);
            continue;
        }
        protocol->frame[protocol->frame_length++] = byte;

        if (protocol->frame_length == WATCH_RESOURCE_PROTOCOL_HEADER_SIZE) {
            if (!header_is_valid(protocol)) {
                ++protocol->frame_error_count;
                emit_nack(protocol, read_u32(protocol->frame + 8U),
                          protocol->frame[4] != WATCH_RESOURCE_PROTOCOL_VERSION
                              ? WATCH_RESOURCE_ERROR_VERSION
                              : read_u16(protocol->frame + 6U) != 0U ? WATCH_RESOURCE_ERROR_FLAGS
                                                                     : WATCH_RESOURCE_ERROR_LENGTH);
                reset_frame(protocol);
                continue;
            }
            protocol->expected_frame_length =
                WATCH_RESOURCE_PROTOCOL_HEADER_SIZE + read_u16(protocol->frame + 12U);
        }
        if (protocol->expected_frame_length != 0U
            && protocol->frame_length == protocol->expected_frame_length) {
            handle_frame(protocol);
            reset_frame(protocol);
        }
    }

    return length;
}

bool watch_resource_protocol_is_active(const watch_resource_protocol_t *protocol)
{
    return protocol != NULL && (protocol->transfer_active || protocol->frame_length != 0U);
}

uint32_t watch_resource_protocol_frame_errors(const watch_resource_protocol_t *protocol)
{
    return protocol == NULL ? 0U : protocol->frame_error_count;
}

uint32_t watch_resource_protocol_accepted_frames(const watch_resource_protocol_t *protocol)
{
    return protocol == NULL ? 0U : protocol->accepted_frame_count;
}

uint32_t watch_resource_protocol_emit_failures(const watch_resource_protocol_t *protocol)
{
    return protocol == NULL ? 0U : protocol->emit_failure_count;
}
