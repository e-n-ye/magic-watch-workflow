#include "watch_ymodem.h"

#include <string.h>

static uint16_t crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0U;

    for (size_t index = 0U; index < length; ++index) {
        crc ^= (uint16_t)data[index] << 8U;
        for (unsigned int bit = 0U; bit < 8U; ++bit) {
            crc = (crc & 0x8000U) != 0U ? (uint16_t)((crc << 1U) ^ 0x1021U) : (uint16_t)(crc << 1U);
        }
    }
    return crc;
}

static bool protocol_valid(const watch_ymodem_t *protocol)
{
    return protocol != NULL && protocol->emit != NULL && protocol->sink.begin != NULL
        && protocol->sink.data != NULL && protocol->sink.commit != NULL
        && protocol->sink.abort != NULL;
}

static bool emit_byte(watch_ymodem_t *protocol, uint8_t byte)
{
    return protocol->emit(protocol->emit_context, byte);
}

static void fail_protocol(watch_ymodem_t *protocol, watch_ymodem_result_t result)
{
    protocol->result = result;
    protocol->state = WATCH_YMODEM_STATE_ERROR;
    if (protocol->file_started) {
        protocol->sink.abort(protocol->sink.context);
        protocol->file_started = false;
    }
    (void)emit_byte(protocol, WATCH_YMODEM_CAN);
    (void)emit_byte(protocol, WATCH_YMODEM_CAN);
}

static bool packet_crc_valid(const watch_ymodem_t *protocol)
{
    uint16_t expected = ((uint16_t)protocol->packet[protocol->packet_expected - 2U] << 8U)
        | protocol->packet[protocol->packet_expected - 1U];
    return crc16(&protocol->packet[3], protocol->block_size) == expected;
}

static bool filename_valid(const uint8_t *data, size_t length)
{
    if (length == 0U || length >= WATCH_YMODEM_MAX_FILENAME) {
        return false;
    }
    for (size_t index = 0U; index < length; ++index) {
        if (data[index] == '/' || data[index] == '\\' || data[index] < 0x20U) {
            return false;
        }
    }
    return true;
}

static bool parse_file_size(const uint8_t *data, size_t length, uint32_t *size)
{
    uint64_t value = 0U;
    bool digit_seen = false;

    for (size_t index = 0U; index < length && data[index] != 0U; ++index) {
        if (data[index] == ' ') {
            break;
        }
        if (data[index] < '0' || data[index] > '9') {
            return false;
        }
        digit_seen = true;
        value = value * 10U + (uint32_t)(data[index] - '0');
        if (value > UINT32_MAX) {
            return false;
        }
    }
    if (!digit_seen || value == 0U) {
        return false;
    }
    *size = (uint32_t)value;
    return true;
}

static void process_header(watch_ymodem_t *protocol)
{
    const uint8_t *payload = &protocol->packet[3];
    const uint8_t *separator = memchr(payload, 0, protocol->block_size);
    size_t name_length;
    size_t remaining;

    if (protocol->packet[1] != 0U || protocol->packet[2] != 0xFFU || separator == NULL) {
        fail_protocol(protocol, WATCH_YMODEM_RESULT_PROTOCOL);
        return;
    }
    name_length = (size_t)(separator - payload);
    if (!filename_valid(payload, name_length)) {
        fail_protocol(protocol, WATCH_YMODEM_RESULT_PROTOCOL);
        return;
    }
    remaining = protocol->block_size - name_length - 1U;
    if (!parse_file_size(separator + 1U, remaining, &protocol->file_size)
        || protocol->file_size > 0x00080000UL) {
        fail_protocol(protocol, WATCH_YMODEM_RESULT_SIZE);
        return;
    }
    memcpy(protocol->filename, payload, name_length);
    protocol->filename[name_length] = '\0';
    if (!protocol->sink.begin(protocol->sink.context, protocol->filename, protocol->file_size)) {
        fail_protocol(protocol, WATCH_YMODEM_RESULT_STORAGE);
        return;
    }
    protocol->file_started = true;
    protocol->file_offset = 0U;
    protocol->expected_block = 1U;
    protocol->state = WATCH_YMODEM_STATE_DATA;
    if (!emit_byte(protocol, WATCH_YMODEM_ACK)) {
        fail_protocol(protocol, WATCH_YMODEM_RESULT_IO);
    }
}

static void process_data(watch_ymodem_t *protocol)
{
    uint8_t block = protocol->packet[1];
    size_t length;

    if (block == (uint8_t)(protocol->expected_block - 1U)) {
        (void)emit_byte(protocol, WATCH_YMODEM_ACK);
        return;
    }
    if (block != protocol->expected_block) {
        fail_protocol(protocol, WATCH_YMODEM_RESULT_SEQUENCE);
        return;
    }
    if (protocol->file_offset >= protocol->file_size) {
        fail_protocol(protocol, WATCH_YMODEM_RESULT_SIZE);
        return;
    }
    length = protocol->file_size - protocol->file_offset;
    if (length > protocol->block_size) {
        length = protocol->block_size;
    }
    if (!protocol->sink.data(protocol->sink.context, protocol->file_offset, &protocol->packet[3],
                             length)) {
        fail_protocol(protocol, WATCH_YMODEM_RESULT_STORAGE);
        return;
    }
    protocol->file_offset += (uint32_t)length;
    protocol->expected_block++;
    if (!emit_byte(protocol, WATCH_YMODEM_ACK)) {
        fail_protocol(protocol, WATCH_YMODEM_RESULT_IO);
    }
}

static void process_packet(watch_ymodem_t *protocol)
{
    if (protocol->packet[1] != (uint8_t)~protocol->packet[2]) {
        fail_protocol(protocol, WATCH_YMODEM_RESULT_SEQUENCE);
        return;
    }
    if (!packet_crc_valid(protocol)) {
        (void)emit_byte(protocol, WATCH_YMODEM_NAK);
        protocol->result = WATCH_YMODEM_RESULT_CRC;
        return;
    }
    protocol->result = WATCH_YMODEM_RESULT_OK;
    if (protocol->state == WATCH_YMODEM_STATE_HEADER) {
        process_header(protocol);
    } else {
        process_data(protocol);
    }
}

static void process_eot(watch_ymodem_t *protocol)
{
    if (protocol->state != WATCH_YMODEM_STATE_DATA
        || protocol->file_offset != protocol->file_size) {
        fail_protocol(protocol, WATCH_YMODEM_RESULT_SIZE);
        return;
    }
    if (!protocol->sink.commit(protocol->sink.context)) {
        fail_protocol(protocol, WATCH_YMODEM_RESULT_STORAGE);
        return;
    }
    protocol->file_started = false;
    protocol->state = WATCH_YMODEM_STATE_COMPLETE;
    protocol->result = WATCH_YMODEM_RESULT_COMPLETE;
    if (!emit_byte(protocol, WATCH_YMODEM_ACK)) {
        protocol->state = WATCH_YMODEM_STATE_ERROR;
        protocol->result = WATCH_YMODEM_RESULT_IO;
    }
}

bool watch_ymodem_init(watch_ymodem_t *protocol, const watch_ymodem_sink_t *sink,
                       watch_ymodem_emit_fn emit, void *emit_context)
{
    if (protocol == NULL || sink == NULL || emit == NULL || sink->begin == NULL
        || sink->data == NULL || sink->commit == NULL || sink->abort == NULL) {
        return false;
    }
    memset(protocol, 0, sizeof(*protocol));
    protocol->sink = *sink;
    protocol->emit = emit;
    protocol->emit_context = emit_context;
    protocol->state = WATCH_YMODEM_STATE_IDLE;
    protocol->result = WATCH_YMODEM_RESULT_OK;
    return true;
}

watch_ymodem_result_t watch_ymodem_start(watch_ymodem_t *protocol)
{
    if (!protocol_valid(protocol)) {
        return WATCH_YMODEM_RESULT_INVALID_ARGUMENT;
    }
    if (protocol->state != WATCH_YMODEM_STATE_IDLE && protocol->state != WATCH_YMODEM_STATE_ERROR
        && protocol->state != WATCH_YMODEM_STATE_COMPLETE) {
        return WATCH_YMODEM_RESULT_BUSY;
    }
    protocol->state = WATCH_YMODEM_STATE_HEADER;
    protocol->result = WATCH_YMODEM_RESULT_OK;
    protocol->packet_length = 0U;
    protocol->packet_expected = 0U;
    protocol->file_started = false;
    return emit_byte(protocol, WATCH_YMODEM_CRC_REQUEST) ? WATCH_YMODEM_RESULT_OK
                                                         : WATCH_YMODEM_RESULT_IO;
}

size_t watch_ymodem_feed(watch_ymodem_t *protocol, const uint8_t *data, size_t length)
{
    if (!protocol_valid(protocol) || (data == NULL && length > 0U)) {
        return 0U;
    }
    for (size_t index = 0U; index < length; ++index) {
        uint8_t byte = data[index];

        if (protocol->state == WATCH_YMODEM_STATE_COMPLETE
            || protocol->state == WATCH_YMODEM_STATE_ERROR) {
            return index;
        }
        if (protocol->packet_length == 0U) {
            if (byte == WATCH_YMODEM_CAN) {
                fail_protocol(protocol, WATCH_YMODEM_RESULT_CANCELLED);
                return index + 1U;
            }
            if (byte == WATCH_YMODEM_EOT) {
                process_eot(protocol);
                continue;
            }
            if (byte == WATCH_YMODEM_SOH) {
                protocol->block_size = 128U;
            } else if (byte == WATCH_YMODEM_STX) {
                protocol->block_size = WATCH_YMODEM_MAX_BLOCK_SIZE;
            } else {
                (void)emit_byte(protocol, WATCH_YMODEM_NAK);
                protocol->result = WATCH_YMODEM_RESULT_PROTOCOL;
                continue;
            }
            protocol->packet[0] = byte;
            protocol->packet_length = 1U;
            protocol->packet_expected = (size_t)protocol->block_size + 5U;
            continue;
        }
        protocol->packet[protocol->packet_length++] = byte;
        if (protocol->packet_length == protocol->packet_expected) {
            process_packet(protocol);
            protocol->packet_length = 0U;
        }
    }
    return length;
}

watch_ymodem_state_t watch_ymodem_state(const watch_ymodem_t *protocol)
{
    return protocol == NULL ? WATCH_YMODEM_STATE_ERROR : protocol->state;
}

watch_ymodem_result_t watch_ymodem_result(const watch_ymodem_t *protocol)
{
    return protocol == NULL ? WATCH_YMODEM_RESULT_INVALID_ARGUMENT : protocol->result;
}

const char *watch_ymodem_result_name(watch_ymodem_result_t result)
{
    switch (result) {
    case WATCH_YMODEM_RESULT_OK:
        return "ok";
    case WATCH_YMODEM_RESULT_COMPLETE:
        return "complete";
    case WATCH_YMODEM_RESULT_INVALID_ARGUMENT:
        return "argument";
    case WATCH_YMODEM_RESULT_BUSY:
        return "busy";
    case WATCH_YMODEM_RESULT_PROTOCOL:
        return "protocol";
    case WATCH_YMODEM_RESULT_CRC:
        return "crc";
    case WATCH_YMODEM_RESULT_SEQUENCE:
        return "sequence";
    case WATCH_YMODEM_RESULT_SIZE:
        return "size";
    case WATCH_YMODEM_RESULT_STORAGE:
        return "storage";
    case WATCH_YMODEM_RESULT_CANCELLED:
        return "cancelled";
    case WATCH_YMODEM_RESULT_IO:
        return "io";
    case WATCH_YMODEM_RESULT_COUNT:
        return "invalid";
    }
    return "invalid";
}
