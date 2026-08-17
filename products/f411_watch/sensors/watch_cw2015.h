#ifndef WATCH_CW2015_H
#define WATCH_CW2015_H

#include <stdbool.h>
#include <stdint.h>

#define WATCH_CW2015_I2C_ADDRESS 0x62U
#define WATCH_CW2015_SERVICE_PERIOD_MS 1000U
#define WATCH_CW2015_SERVICE_EVENT_SAMPLE 0x0104U

#define WATCH_CW2015_REG_VERSION 0x00U
#define WATCH_CW2015_REG_VCELL 0x02U
#define WATCH_CW2015_REG_SOC 0x04U
#define WATCH_CW2015_REG_CONFIG 0x08U
#define WATCH_CW2015_REG_MODE 0x0AU
#define WATCH_CW2015_SOC_INVALID 0xFFU

typedef enum {
    WATCH_CW2015_RESULT_OK = 0,
    WATCH_CW2015_RESULT_INVALID_ARGUMENT,
    WATCH_CW2015_RESULT_BUS_ERROR,
    WATCH_CW2015_RESULT_INVALID_SOC,
    WATCH_CW2015_RESULT_COUNT
} watch_cw2015_result_t;

typedef enum {
    WATCH_CW2015_SERVICE_STATE_NEW = 0,
    WATCH_CW2015_SERVICE_STATE_READY,
    WATCH_CW2015_SERVICE_STATE_FAILED,
    WATCH_CW2015_SERVICE_STATE_COUNT
} watch_cw2015_service_state_t;

typedef bool (*watch_cw2015_read_fn)(void *context, uint8_t address, uint8_t reg, uint8_t *data,
                                     uint8_t length);

typedef struct
{
    watch_cw2015_read_fn read;
    void *context;
} watch_cw2015_bus_t;

typedef struct
{
    uint16_t voltage_mv;
    uint8_t soc_percent;
    uint8_t soc_fraction;
} watch_cw2015_sample_t;

typedef bool (*watch_cw2015_publish_fn)(void *context, uint32_t timestamp_ms);

typedef struct
{
    watch_cw2015_bus_t bus;
    watch_cw2015_publish_fn publish;
    void *publish_context;
    watch_cw2015_sample_t latest_sample;
    uint32_t last_sample_ms;
    uint32_t sample_count;
    uint32_t read_error_count;
    uint32_t invalid_soc_count;
    uint32_t event_drop_count;
    uint8_t version;
    watch_cw2015_service_state_t state;
    bool initialized;
    bool ready;
    bool sample_valid;
} watch_cw2015_service_t;

typedef struct
{
    bool ready;
    bool sample_valid;
    uint8_t version;
    uint8_t soc_percent;
    uint8_t soc_fraction;
    uint16_t voltage_mv;
    uint32_t last_sample_ms;
    uint32_t sample_count;
    uint32_t read_error_count;
    uint32_t invalid_soc_count;
    uint32_t event_drop_count;
    watch_cw2015_service_state_t state;
} watch_cw2015_service_status_t;

bool watch_cw2015_decode_voltage(const uint8_t data[2], uint16_t *voltage_mv);
bool watch_cw2015_decode_soc(const uint8_t data[2], uint8_t *soc_percent, uint8_t *soc_fraction);
bool watch_cw2015_service_init(watch_cw2015_service_t *service, const watch_cw2015_bus_t *bus,
                               watch_cw2015_publish_fn publish, void *publish_context);
bool watch_cw2015_service_process(watch_cw2015_service_t *service, uint32_t now_ms);
bool watch_cw2015_service_read_status(const watch_cw2015_service_t *service,
                                      watch_cw2015_service_status_t *status);
bool watch_cw2015_service_read_latest(const watch_cw2015_service_t *service,
                                      watch_cw2015_sample_t *sample);

#endif /* WATCH_CW2015_H */
