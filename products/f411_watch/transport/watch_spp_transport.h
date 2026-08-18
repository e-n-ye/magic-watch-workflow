#ifndef WATCH_SPP_TRANSPORT_H
#define WATCH_SPP_TRANSPORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WATCH_SPP_RX_RING_SIZE 1024U
#define WATCH_SPP_TX_RING_SIZE 1024U
#define WATCH_SPP_TX_CHUNK_SIZE 64U
#define WATCH_SPP_LINK_TIMEOUT_MS 3000U

typedef enum {
    WATCH_SPP_STATE_DISABLED = 0,
    WATCH_SPP_STATE_READY,
    WATCH_SPP_STATE_ACTIVE,
    WATCH_SPP_STATE_FAULT,
    WATCH_SPP_STATE_COUNT
} watch_spp_state_t;

typedef struct
{
    watch_spp_state_t state;
    bool enabled;
    bool rx_armed;
    bool tx_in_flight;
    uint32_t last_rx_ms;
    uint32_t rx_bytes;
    uint32_t tx_bytes;
    uint32_t rx_dropped;
    uint32_t tx_dropped;
    uint32_t idle_events;
    uint32_t rx_errors;
    uint32_t tx_errors;
    uint32_t disconnects;
    uint32_t recovery_count;
} watch_spp_status_t;

typedef struct
{
    uint8_t rx_ring[WATCH_SPP_RX_RING_SIZE];
    uint8_t tx_ring[WATCH_SPP_TX_RING_SIZE];
    uint16_t rx_head;
    uint16_t rx_tail;
    uint16_t tx_head;
    uint16_t tx_tail;
    uint16_t tx_in_flight_length;
    watch_spp_status_t status;
} watch_spp_transport_t;

void watch_spp_transport_init(watch_spp_transport_t *transport);
void watch_spp_transport_set_enabled(watch_spp_transport_t *transport, bool enabled);
void watch_spp_transport_set_rx_armed(watch_spp_transport_t *transport, bool armed);
void watch_spp_transport_on_dma_idle(watch_spp_transport_t *transport, const uint8_t *data,
                                     size_t length, uint32_t now_ms);
void watch_spp_transport_on_rx_error(watch_spp_transport_t *transport);
void watch_spp_transport_on_tx_started(watch_spp_transport_t *transport, size_t length);
void watch_spp_transport_on_tx_complete(watch_spp_transport_t *transport, bool success);
void watch_spp_transport_on_recovery(watch_spp_transport_t *transport);
void watch_spp_transport_process(watch_spp_transport_t *transport, uint32_t now_ms);

size_t watch_spp_transport_read(watch_spp_transport_t *transport, uint8_t *data, size_t length);
size_t watch_spp_transport_write(watch_spp_transport_t *transport, const uint8_t *data,
                                 size_t length);
size_t watch_spp_transport_tx_peek(const watch_spp_transport_t *transport, const uint8_t **data);
bool watch_spp_transport_read_status(const watch_spp_transport_t *transport,
                                     watch_spp_status_t *status);

#endif /* WATCH_SPP_TRANSPORT_H */
