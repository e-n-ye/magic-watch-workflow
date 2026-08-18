#include "watch_spp_transport.h"

#include <string.h>

static uint16_t next_index(uint16_t index, uint16_t capacity)
{
    ++index;
    return index == capacity ? 0U : index;
}

static size_t ring_count(uint16_t head, uint16_t tail, uint16_t capacity)
{
    return head >= tail ? (size_t)(head - tail) : (size_t)(capacity - tail + head);
}

static size_t ring_space(uint16_t head, uint16_t tail, uint16_t capacity)
{
    return (size_t)capacity - 1U - ring_count(head, tail, capacity);
}

static void set_state(watch_spp_transport_t *transport, watch_spp_state_t state)
{
    transport->status.state = state;
}

void watch_spp_transport_init(watch_spp_transport_t *transport)
{
    if (transport == NULL) {
        return;
    }

    memset(transport, 0, sizeof(*transport));
    set_state(transport, WATCH_SPP_STATE_DISABLED);
}

void watch_spp_transport_set_enabled(watch_spp_transport_t *transport, bool enabled)
{
    if (transport == NULL) {
        return;
    }

    transport->status.enabled = enabled;
    transport->status.rx_armed = false;
    transport->status.tx_in_flight = false;
    transport->tx_in_flight_length = 0U;
    if (!enabled) {
        transport->rx_head = 0U;
        transport->rx_tail = 0U;
        transport->tx_head = 0U;
        transport->tx_tail = 0U;
        set_state(transport, WATCH_SPP_STATE_DISABLED);
    } else {
        set_state(transport, WATCH_SPP_STATE_READY);
    }
}

void watch_spp_transport_set_rx_armed(watch_spp_transport_t *transport, bool armed)
{
    if (transport == NULL) {
        return;
    }

    transport->status.rx_armed = armed;
    if (transport->status.enabled && !armed) {
        set_state(transport, WATCH_SPP_STATE_FAULT);
    } else if (transport->status.enabled && transport->status.state == WATCH_SPP_STATE_FAULT) {
        set_state(transport, WATCH_SPP_STATE_READY);
    }
}

void watch_spp_transport_on_dma_idle(watch_spp_transport_t *transport, const uint8_t *data,
                                     size_t length, uint32_t now_ms)
{
    size_t accepted;

    if (transport == NULL || !transport->status.enabled || data == NULL || length == 0U) {
        return;
    }

    ++transport->status.idle_events;
    accepted = length;
    if (accepted > ring_space(transport->rx_head, transport->rx_tail, WATCH_SPP_RX_RING_SIZE)) {
        accepted = ring_space(transport->rx_head, transport->rx_tail, WATCH_SPP_RX_RING_SIZE);
    }

    for (size_t index = 0U; index < accepted; ++index) {
        transport->rx_ring[transport->rx_head] = data[index];
        transport->rx_head = next_index(transport->rx_head, WATCH_SPP_RX_RING_SIZE);
    }

    transport->status.rx_bytes += (uint32_t)accepted;
    transport->status.rx_dropped += (uint32_t)(length - accepted);
    transport->status.last_rx_ms = now_ms;
    set_state(transport, WATCH_SPP_STATE_ACTIVE);
}

void watch_spp_transport_on_rx_error(watch_spp_transport_t *transport)
{
    if (transport == NULL) {
        return;
    }

    ++transport->status.rx_errors;
    if (transport->status.enabled) {
        set_state(transport, WATCH_SPP_STATE_FAULT);
    }
}

void watch_spp_transport_on_tx_started(watch_spp_transport_t *transport, size_t length)
{
    if (transport == NULL || !transport->status.enabled || transport->status.tx_in_flight
        || length == 0U || length > WATCH_SPP_TX_CHUNK_SIZE
        || length > ring_count(transport->tx_head, transport->tx_tail, WATCH_SPP_TX_RING_SIZE)) {
        return;
    }

    transport->status.tx_in_flight = true;
    transport->tx_in_flight_length = (uint16_t)length;
}

void watch_spp_transport_on_tx_complete(watch_spp_transport_t *transport, bool success)
{
    if (transport == NULL || !transport->status.tx_in_flight) {
        return;
    }

    if (success) {
        for (uint16_t index = 0U; index < transport->tx_in_flight_length; ++index) {
            transport->tx_tail = next_index(transport->tx_tail, WATCH_SPP_TX_RING_SIZE);
        }
        transport->status.tx_bytes += transport->tx_in_flight_length;
    } else {
        ++transport->status.tx_errors;
        if (transport->status.enabled) {
            set_state(transport, WATCH_SPP_STATE_FAULT);
        }
    }

    transport->status.tx_in_flight = false;
    transport->tx_in_flight_length = 0U;
}

void watch_spp_transport_on_recovery(watch_spp_transport_t *transport)
{
    if (transport == NULL) {
        return;
    }

    ++transport->status.recovery_count;
    transport->status.rx_armed = false;
    transport->status.tx_in_flight = false;
    transport->tx_in_flight_length = 0U;
    if (transport->status.enabled) {
        set_state(transport, WATCH_SPP_STATE_READY);
    }
}

void watch_spp_transport_process(watch_spp_transport_t *transport, uint32_t now_ms)
{
    if (transport == NULL || !transport->status.enabled) {
        return;
    }

    if (transport->status.state == WATCH_SPP_STATE_ACTIVE
        && (uint32_t)(now_ms - transport->status.last_rx_ms) >= WATCH_SPP_LINK_TIMEOUT_MS) {
        ++transport->status.disconnects;
        set_state(transport, WATCH_SPP_STATE_READY);
    }
}

size_t watch_spp_transport_read(watch_spp_transport_t *transport, uint8_t *data, size_t length)
{
    size_t available;

    if (transport == NULL || data == NULL || length == 0U) {
        return 0U;
    }

    available = ring_count(transport->rx_head, transport->rx_tail, WATCH_SPP_RX_RING_SIZE);
    if (length > available) {
        length = available;
    }
    for (size_t index = 0U; index < length; ++index) {
        data[index] = transport->rx_ring[transport->rx_tail];
        transport->rx_tail = next_index(transport->rx_tail, WATCH_SPP_RX_RING_SIZE);
    }
    return length;
}

size_t watch_spp_transport_write(watch_spp_transport_t *transport, const uint8_t *data,
                                 size_t length)
{
    size_t accepted;

    if (transport == NULL || data == NULL || length == 0U || !transport->status.enabled) {
        return 0U;
    }

    accepted = length;
    if (accepted > ring_space(transport->tx_head, transport->tx_tail, WATCH_SPP_TX_RING_SIZE)) {
        accepted = ring_space(transport->tx_head, transport->tx_tail, WATCH_SPP_TX_RING_SIZE);
    }
    for (size_t index = 0U; index < accepted; ++index) {
        transport->tx_ring[transport->tx_head] = data[index];
        transport->tx_head = next_index(transport->tx_head, WATCH_SPP_TX_RING_SIZE);
    }
    transport->status.tx_dropped += (uint32_t)(length - accepted);
    return accepted;
}

size_t watch_spp_transport_tx_peek(const watch_spp_transport_t *transport, const uint8_t **data)
{
    size_t available;
    size_t contiguous;

    if (transport == NULL || data == NULL || transport->status.tx_in_flight) {
        return 0U;
    }

    available = ring_count(transport->tx_head, transport->tx_tail, WATCH_SPP_TX_RING_SIZE);
    if (available == 0U) {
        return 0U;
    }
    contiguous = transport->tx_head > transport->tx_tail
        ? (size_t)(transport->tx_head - transport->tx_tail)
        : (size_t)(WATCH_SPP_TX_RING_SIZE - transport->tx_tail);
    if (contiguous > WATCH_SPP_TX_CHUNK_SIZE) {
        contiguous = WATCH_SPP_TX_CHUNK_SIZE;
    }
    *data = &transport->tx_ring[transport->tx_tail];
    return contiguous;
}

bool watch_spp_transport_read_status(const watch_spp_transport_t *transport,
                                     watch_spp_status_t *status)
{
    if (transport == NULL || status == NULL) {
        return false;
    }

    *status = transport->status;
    return true;
}
