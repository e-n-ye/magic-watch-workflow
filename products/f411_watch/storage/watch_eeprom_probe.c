#include "watch_eeprom_probe.h"

#include <stddef.h>

bool watch_eeprom_probe_init(watch_eeprom_probe_service_t *service,
                             watch_eeprom_probe_is_ready_fn is_ready, void *context)
{
    if (service == NULL || is_ready == NULL) {
        return false;
    }

    *service = (watch_eeprom_probe_service_t) {
        .is_ready = is_ready,
        .context = context,
        .initialized = true,
    };
    return true;
}

bool watch_eeprom_probe_process(watch_eeprom_probe_service_t *service)
{
    uint8_t address;

    if (service == NULL || !service->initialized || service->complete) {
        return false;
    }

    address = (uint8_t)(WATCH_EEPROM_PROBE_FIRST_ADDRESS + service->next_index);
    if (service->is_ready(service->context, address)) {
        service->response_mask |= (uint8_t)(1U << service->next_index);
    }

    service->next_index++;
    service->probed_count++;
    if (service->probed_count >= WATCH_EEPROM_PROBE_ADDRESS_COUNT) {
        service->complete = true;
        service->scan_count++;
    }

    return true;
}

bool watch_eeprom_probe_read_status(const watch_eeprom_probe_service_t *service,
                                    watch_eeprom_probe_status_t *status)
{
    if (service == NULL || status == NULL || !service->initialized) {
        return false;
    }

    *status = (watch_eeprom_probe_status_t) {
        .complete = service->complete,
        .first_address = WATCH_EEPROM_PROBE_FIRST_ADDRESS,
        .last_address = WATCH_EEPROM_PROBE_LAST_ADDRESS,
        .probed_count = service->probed_count,
        .response_mask = service->response_mask,
        .scan_count = service->scan_count,
    };
    return true;
}
