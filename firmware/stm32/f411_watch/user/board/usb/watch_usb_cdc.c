#include "watch_usb_cdc.h"

#include <stdbool.h>

#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_cdc_if.h"

extern USBD_HandleTypeDef hUsbDeviceFS;

#define WATCH_USB_CDC_RX_CAPACITY 1024U
#define WATCH_USB_CDC_TX_CAPACITY 1024U
#define WATCH_USB_CDC_PACKET_SIZE 64U

static uint8_t s_rx_ring[WATCH_USB_CDC_RX_CAPACITY];
static volatile uint16_t s_rx_head;
static volatile uint16_t s_rx_tail;
static volatile uint32_t s_rx_dropped;

static uint8_t s_tx_ring[WATCH_USB_CDC_TX_CAPACITY];
static uint16_t s_tx_head;
static uint16_t s_tx_tail;
static uint32_t s_tx_dropped;
static uint8_t s_tx_packet[WATCH_USB_CDC_PACKET_SIZE];
static volatile bool s_tx_in_flight;
static volatile bool s_initialized;

static uint16_t ring_next(uint16_t index, uint16_t capacity)
{
    ++index;
    if (index == capacity) {
        index = 0U;
    }
    return index;
}

void watch_usb_cdc_reset(void)
{
    s_rx_head = 0U;
    s_rx_tail = 0U;
    s_rx_dropped = 0U;
    s_tx_head = 0U;
    s_tx_tail = 0U;
    s_tx_dropped = 0U;
    s_tx_in_flight = false;
    s_initialized = true;
}

void watch_usb_cdc_stop(void)
{
    s_initialized = false;
    s_tx_in_flight = false;
}

void watch_usb_cdc_reinitialize(void)
{
    (void)USBD_Stop(&hUsbDeviceFS);
    (void)USBD_DeInit(&hUsbDeviceFS);
    MX_USB_DEVICE_Init();
}

void watch_usb_cdc_on_receive(const uint8_t *data, uint32_t length)
{
    if ((data == 0) || !s_initialized) {
        return;
    }

    for (uint32_t index = 0U; index < length; ++index) {
        uint16_t next = ring_next(s_rx_head, WATCH_USB_CDC_RX_CAPACITY);
        if (next == s_rx_tail) {
            ++s_rx_dropped;
            continue;
        }

        s_rx_ring[s_rx_head] = data[index];
        s_rx_head = next;
    }
}

void watch_usb_cdc_on_transmit_complete(void)
{
    s_tx_in_flight = false;
}

size_t watch_usb_cdc_read(uint8_t *data, size_t capacity)
{
    size_t length = 0U;

    if (data == 0) {
        return 0U;
    }

    while ((length < capacity) && (s_rx_tail != s_rx_head)) {
        data[length] = s_rx_ring[s_rx_tail];
        s_rx_tail = ring_next(s_rx_tail, WATCH_USB_CDC_RX_CAPACITY);
        ++length;
    }

    return length;
}

size_t watch_usb_cdc_write(const uint8_t *data, size_t length)
{
    size_t accepted = 0U;

    if (data == 0) {
        return 0U;
    }

    while (accepted < length) {
        uint16_t next = ring_next(s_tx_head, WATCH_USB_CDC_TX_CAPACITY);
        if (next == s_tx_tail) {
            break;
        }

        s_tx_ring[s_tx_head] = data[accepted];
        s_tx_head = next;
        ++accepted;
    }

    s_tx_dropped += (uint32_t)(length - accepted);
    return accepted;
}

void watch_usb_cdc_process(void)
{
    size_t length = 0U;
    uint16_t next_tail;

    if (!s_initialized || s_tx_in_flight || (s_tx_tail == s_tx_head)) {
        return;
    }

    next_tail = s_tx_tail;
    while ((length < sizeof(s_tx_packet)) && (next_tail != s_tx_head)) {
        s_tx_packet[length] = s_tx_ring[next_tail];
        next_tail = ring_next(next_tail, WATCH_USB_CDC_TX_CAPACITY);
        ++length;
    }

    s_tx_in_flight = true;
    if (CDC_Transmit_FS(s_tx_packet, (uint16_t)length) == USBD_OK) {
        s_tx_tail = next_tail;
        return;
    }

    s_tx_in_flight = false;
}

size_t watch_usb_cdc_rx_pending(void)
{
    uint16_t head = s_rx_head;

    if (head >= s_rx_tail) {
        return (size_t)(head - s_rx_tail);
    }

    return (size_t)(WATCH_USB_CDC_RX_CAPACITY - s_rx_tail + head);
}

uint32_t watch_usb_cdc_rx_dropped(void)
{
    return s_rx_dropped;
}

uint32_t watch_usb_cdc_tx_dropped(void)
{
    return s_tx_dropped;
}
