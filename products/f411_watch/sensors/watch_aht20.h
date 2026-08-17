#ifndef WATCH_AHT20_H
#define WATCH_AHT20_H

#include <stdbool.h>
#include <stdint.h>

#define WATCH_AHT20_I2C_ADDRESS 0x38U
#define WATCH_AHT20_SERVICE_PERIOD_MS 1000U
#define WATCH_AHT20_INIT_TIMEOUT_MS 100U
#define WATCH_AHT20_MEASUREMENT_TIMEOUT_MS 100U
#define WATCH_AHT20_SERVICE_EVENT_SAMPLE 0x0102U
#define WATCH_AHT20_STATUS_BUSY 0x80U
#define WATCH_AHT20_STATUS_CALIBRATED 0x08U
#define WATCH_AHT20_MEASUREMENT_RESPONSE_LENGTH 7U

#define WATCH_AHT20_COMMAND_INITIALIZE 0xBEU
#define WATCH_AHT20_COMMAND_TRIGGER 0xACU

typedef enum {
    WATCH_AHT20_RESULT_OK = 0,
    WATCH_AHT20_RESULT_INVALID_ARGUMENT,
    WATCH_AHT20_RESULT_BUS_ERROR,
    WATCH_AHT20_RESULT_BUSY_TIMEOUT,
    WATCH_AHT20_RESULT_CRC_ERROR,
    WATCH_AHT20_RESULT_DATA_ERROR,
    WATCH_AHT20_RESULT_NOT_READY,
    WATCH_AHT20_RESULT_COUNT
} watch_aht20_result_t;

typedef enum {
    WATCH_AHT20_SERVICE_STATE_NEW = 0,
    WATCH_AHT20_SERVICE_STATE_INIT_WAIT,
    WATCH_AHT20_SERVICE_STATE_MEASUREMENT_WAIT,
    WATCH_AHT20_SERVICE_STATE_READY,
    WATCH_AHT20_SERVICE_STATE_FAILED,
    WATCH_AHT20_SERVICE_STATE_COUNT
} watch_aht20_service_state_t;

typedef bool (*watch_aht20_write_fn)(void *context, uint8_t address, const uint8_t *data,
                                     uint8_t length);
typedef bool (*watch_aht20_read_fn)(void *context, uint8_t address, uint8_t *data, uint8_t length);

typedef struct
{
    watch_aht20_write_fn write;
    watch_aht20_read_fn read;
    void *context;
} watch_aht20_bus_t;

typedef struct
{
    int16_t temperature_centi_c;
    uint16_t humidity_centi_percent;
} watch_aht20_sample_t;

typedef bool (*watch_aht20_publish_fn)(void *context, uint32_t timestamp_ms);

typedef struct
{
    watch_aht20_bus_t bus;
    watch_aht20_publish_fn publish;
    void *publish_context;
    watch_aht20_sample_t latest_sample;
    uint32_t deadline_ms;
    uint32_t last_sample_ms;
    uint32_t sample_count;
    uint32_t read_error_count;
    uint32_t crc_error_count;
    uint32_t timeout_count;
    uint32_t event_drop_count;
    uint8_t status_byte;
    watch_aht20_service_state_t state;
    bool initialized;
    bool calibrated;
    bool ready;
    bool sample_valid;
} watch_aht20_service_t;

typedef struct
{
    bool ready;
    bool calibrated;
    bool sample_valid;
    uint8_t status_byte;
    uint32_t last_sample_ms;
    uint32_t sample_count;
    uint32_t read_error_count;
    uint32_t crc_error_count;
    uint32_t timeout_count;
    uint32_t event_drop_count;
    watch_aht20_service_state_t state;
} watch_aht20_service_status_t;

bool watch_aht20_validate_crc(const uint8_t data[WATCH_AHT20_MEASUREMENT_RESPONSE_LENGTH]);
bool watch_aht20_decode_measurement(const uint8_t data[WATCH_AHT20_MEASUREMENT_RESPONSE_LENGTH],
                                    watch_aht20_sample_t *sample);
bool watch_aht20_service_init(watch_aht20_service_t *service, const watch_aht20_bus_t *bus,
                              watch_aht20_publish_fn publish, void *publish_context);
bool watch_aht20_service_process(watch_aht20_service_t *service, uint32_t now_ms);
bool watch_aht20_service_read_status(const watch_aht20_service_t *service,
                                     watch_aht20_service_status_t *status);
bool watch_aht20_service_read_latest(const watch_aht20_service_t *service,
                                     watch_aht20_sample_t *sample);

#endif /* WATCH_AHT20_H */
