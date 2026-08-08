#ifndef WATCH_USB_CDC_H
#define WATCH_USB_CDC_H

#include <stddef.h>
#include <stdint.h>

void watch_usb_cdc_reset(void);
void watch_usb_cdc_stop(void);
void watch_usb_cdc_on_receive(const uint8_t *data, uint32_t length);
void watch_usb_cdc_on_transmit_complete(void);
size_t watch_usb_cdc_read(uint8_t *data, size_t capacity);
size_t watch_usb_cdc_write(const uint8_t *data, size_t length);
void watch_usb_cdc_process(void);
size_t watch_usb_cdc_rx_pending(void);
uint32_t watch_usb_cdc_rx_dropped(void);
uint32_t watch_usb_cdc_tx_dropped(void);

#endif /* WATCH_USB_CDC_H */
