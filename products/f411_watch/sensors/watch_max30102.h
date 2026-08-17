#ifndef WATCH_MAX30102_H
#define WATCH_MAX30102_H

#include <stdbool.h>
#include <stdint.h>

#define WATCH_MAX30102_I2C_ADDRESS 0x57U
#define WATCH_MAX30102_PART_ID 0x15U
#define WATCH_MAX30102_SERVICE_PERIOD_MS 40U
#define WATCH_MAX30102_RESET_TIMEOUT_MS 100U
#define WATCH_MAX30102_FIFO_DEPTH 32U
#define WATCH_MAX30102_FINGER_THRESHOLD 10000U
#define WATCH_MAX30102_SERVICE_EVENT_SAMPLE 0x0103U

#define WATCH_MAX30102_REG_INT_STATUS1 0x00U
#define WATCH_MAX30102_REG_INT_STATUS2 0x01U
#define WATCH_MAX30102_REG_INT_ENABLE1 0x02U
#define WATCH_MAX30102_REG_INT_ENABLE2 0x03U
#define WATCH_MAX30102_REG_FIFO_WR_PTR 0x04U
#define WATCH_MAX30102_REG_FIFO_OVF_COUNTER 0x05U
#define WATCH_MAX30102_REG_FIFO_RD_PTR 0x06U
#define WATCH_MAX30102_REG_FIFO_DATA 0x07U
#define WATCH_MAX30102_REG_FIFO_CONFIG 0x08U
#define WATCH_MAX30102_REG_MODE_CONFIG 0x09U
#define WATCH_MAX30102_REG_SPO2_CONFIG 0x0AU
#define WATCH_MAX30102_REG_LED1_PA 0x0CU
#define WATCH_MAX30102_REG_LED2_PA 0x0DU
#define WATCH_MAX30102_REG_REV_ID 0xFEU
#define WATCH_MAX30102_REG_PART_ID 0xFFU

#define WATCH_MAX30102_MODE_RESET 0x40U
#define WATCH_MAX30102_MODE_SPO2 0x03U

typedef enum {
    WATCH_MAX30102_RESULT_OK = 0,
    WATCH_MAX30102_RESULT_INVALID_ARGUMENT,
    WATCH_MAX30102_RESULT_BUS_ERROR,
    WATCH_MAX30102_RESULT_ID_MISMATCH,
    WATCH_MAX30102_RESULT_RESET_TIMEOUT,
    WATCH_MAX30102_RESULT_COUNT
} watch_max30102_result_t;

typedef enum {
    WATCH_MAX30102_SERVICE_STATE_NEW = 0,
    WATCH_MAX30102_SERVICE_STATE_RESET_WAIT,
    WATCH_MAX30102_SERVICE_STATE_CLEAR_WRITE_PTR,
    WATCH_MAX30102_SERVICE_STATE_CLEAR_OVERFLOW,
    WATCH_MAX30102_SERVICE_STATE_CLEAR_READ_PTR,
    WATCH_MAX30102_SERVICE_STATE_CONFIG_FIFO,
    WATCH_MAX30102_SERVICE_STATE_CONFIG_SPO2,
    WATCH_MAX30102_SERVICE_STATE_CONFIG_RED,
    WATCH_MAX30102_SERVICE_STATE_CONFIG_IR,
    WATCH_MAX30102_SERVICE_STATE_CONFIG_INTERRUPT,
    WATCH_MAX30102_SERVICE_STATE_CONFIG_MODE,
    WATCH_MAX30102_SERVICE_STATE_READY,
    WATCH_MAX30102_SERVICE_STATE_FAILED,
    WATCH_MAX30102_SERVICE_STATE_COUNT
} watch_max30102_service_state_t;

typedef bool (*watch_max30102_read_fn)(void *context, uint8_t address, uint8_t reg, uint8_t *data,
                                       uint8_t length);
typedef bool (*watch_max30102_write_fn)(void *context, uint8_t address, uint8_t reg,
                                        const uint8_t *data, uint8_t length);

typedef struct
{
    watch_max30102_read_fn read;
    watch_max30102_write_fn write;
    void *context;
} watch_max30102_bus_t;

typedef struct
{
    uint32_t red_raw;
    uint32_t ir_raw;
    bool finger_on;
} watch_max30102_sample_t;

typedef bool (*watch_max30102_publish_fn)(void *context, uint32_t timestamp_ms);

typedef struct
{
    watch_max30102_bus_t bus;
    watch_max30102_publish_fn publish;
    void *publish_context;
    watch_max30102_sample_t latest_sample;
    uint32_t deadline_ms;
    uint32_t last_sample_ms;
    uint32_t sample_count;
    uint32_t no_data_count;
    uint32_t read_error_count;
    uint32_t id_error_count;
    uint32_t reset_timeout_count;
    uint32_t fifo_overflow_count;
    uint32_t event_drop_count;
    uint8_t part_id;
    uint8_t revision_id;
    uint8_t mode_config;
    uint8_t fifo_write_pointer;
    uint8_t fifo_read_pointer;
    watch_max30102_service_state_t state;
    bool initialized;
    bool ready;
    bool sample_valid;
} watch_max30102_service_t;

typedef struct
{
    bool ready;
    uint8_t part_id;
    uint8_t revision_id;
    uint8_t mode_config;
    bool sample_valid;
    uint32_t last_sample_ms;
    uint32_t sample_count;
    uint32_t no_data_count;
    uint32_t read_error_count;
    uint32_t id_error_count;
    uint32_t reset_timeout_count;
    uint32_t fifo_overflow_count;
    uint32_t event_drop_count;
    watch_max30102_service_state_t state;
} watch_max30102_service_status_t;

bool watch_max30102_validate_identity(uint8_t part_id);
bool watch_max30102_decode_fifo_sample(const uint8_t data[6], watch_max30102_sample_t *sample);
bool watch_max30102_service_init(watch_max30102_service_t *service, const watch_max30102_bus_t *bus,
                                 watch_max30102_publish_fn publish, void *publish_context);
bool watch_max30102_service_process(watch_max30102_service_t *service, uint32_t now_ms);
bool watch_max30102_service_read_status(const watch_max30102_service_t *service,
                                        watch_max30102_service_status_t *status);
bool watch_max30102_service_read_latest(const watch_max30102_service_t *service,
                                        watch_max30102_sample_t *sample);

#endif /* WATCH_MAX30102_H */
