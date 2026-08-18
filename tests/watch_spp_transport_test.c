#include "watch_spp_transport.h"

#include <assert.h>
#include <string.h>

static void test_dma_ring_and_disconnect(void)
{
    watch_spp_transport_t transport;
    watch_spp_status_t status;
    uint8_t source[80];
    uint8_t output[80];

    for (size_t index = 0U; index < sizeof(source); ++index) {
        source[index] = (uint8_t)(index + 1U);
    }
    watch_spp_transport_init(&transport);
    watch_spp_transport_set_enabled(&transport, true);
    watch_spp_transport_set_rx_armed(&transport, true);
    watch_spp_transport_on_dma_idle(&transport, source, sizeof(source), 100U);
    assert(watch_spp_transport_read(&transport, output, sizeof(output)) == sizeof(output));
    assert(memcmp(source, output, sizeof(source)) == 0);
    assert(watch_spp_transport_read_status(&transport, &status));
    assert(status.state == WATCH_SPP_STATE_ACTIVE && status.rx_bytes == sizeof(source));
    watch_spp_transport_process(&transport, 100U + WATCH_SPP_LINK_TIMEOUT_MS);
    assert(watch_spp_transport_read_status(&transport, &status));
    assert(status.state == WATCH_SPP_STATE_READY && status.disconnects == 1U);
}

static void test_overflow_and_tx_commit(void)
{
    watch_spp_transport_t transport;
    watch_spp_status_t status;
    uint8_t source[WATCH_SPP_RX_RING_SIZE + 20U];
    uint8_t tx[130];
    const uint8_t *chunk;
    size_t length;

    memset(source, 0xA5, sizeof(source));
    memset(tx, 0x5A, sizeof(tx));
    watch_spp_transport_init(&transport);
    watch_spp_transport_set_enabled(&transport, true);
    watch_spp_transport_set_rx_armed(&transport, true);
    watch_spp_transport_on_dma_idle(&transport, source, sizeof(source), 1U);
    assert(watch_spp_transport_read_status(&transport, &status));
    assert(status.rx_dropped == 21U);
    assert(watch_spp_transport_write(&transport, tx, sizeof(tx)) == sizeof(tx));
    length = watch_spp_transport_tx_peek(&transport, &chunk);
    assert(length == WATCH_SPP_TX_CHUNK_SIZE && chunk[0] == 0x5A);
    watch_spp_transport_on_tx_started(&transport, length);
    assert(watch_spp_transport_tx_peek(&transport, &chunk) == 0U);
    watch_spp_transport_on_tx_complete(&transport, true);
    assert(watch_spp_transport_tx_peek(&transport, &chunk) == WATCH_SPP_TX_CHUNK_SIZE);
    watch_spp_transport_on_tx_started(&transport, WATCH_SPP_TX_CHUNK_SIZE);
    watch_spp_transport_on_tx_complete(&transport, false);
    assert(watch_spp_transport_read_status(&transport, &status));
    assert(status.tx_bytes == WATCH_SPP_TX_CHUNK_SIZE && status.tx_errors == 1U);
    assert(status.state == WATCH_SPP_STATE_FAULT);
}

static void test_disable_and_recovery(void)
{
    watch_spp_transport_t transport;
    watch_spp_status_t status;
    uint8_t byte = 0x42;

    watch_spp_transport_init(&transport);
    watch_spp_transport_set_enabled(&transport, true);
    watch_spp_transport_set_rx_armed(&transport, true);
    watch_spp_transport_on_dma_idle(&transport, &byte, 1U, 10U);
    watch_spp_transport_on_rx_error(&transport);
    assert(watch_spp_transport_read_status(&transport, &status));
    assert(status.state == WATCH_SPP_STATE_FAULT && status.rx_errors == 1U);
    watch_spp_transport_on_recovery(&transport);
    assert(watch_spp_transport_read_status(&transport, &status));
    assert(status.state == WATCH_SPP_STATE_READY && status.recovery_count == 1U);
    watch_spp_transport_set_enabled(&transport, false);
    assert(watch_spp_transport_write(&transport, &byte, 1U) == 0U);
    assert(watch_spp_transport_read_status(&transport, &status));
    assert(status.state == WATCH_SPP_STATE_DISABLED && !status.enabled);
}

int main(void)
{
    test_dma_ring_and_disconnect();
    test_overflow_and_tx_commit();
    test_disable_and_recovery();
    return 0;
}
