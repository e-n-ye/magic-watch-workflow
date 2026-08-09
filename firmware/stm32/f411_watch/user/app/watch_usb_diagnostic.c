#include "watch_usb_diagnostic.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "board/usb/watch_usb_cdc.h"
#include "main.h"
#include "watch_app.h"
#include "watch_diagnostic.h"
#include "watch_runtime.h"
#include "ui/watch_ui.h"

#define WATCH_USB_DIAGNOSTIC_COMMAND_SIZE 96U
#define WATCH_USB_DIAGNOSTIC_READ_SIZE 32U
#define WATCH_USB_DIAGNOSTIC_EVENT_COMMAND 1U

typedef enum {
    WATCH_USB_COMMAND_HELP = 1,
    WATCH_USB_COMMAND_PING,
    WATCH_USB_COMMAND_INFO,
    WATCH_USB_COMMAND_DIAG,
    WATCH_USB_COMMAND_STATS,
    WATCH_USB_COMMAND_HEALTH,
    WATCH_USB_COMMAND_UNKNOWN,
    WATCH_USB_COMMAND_COUNT
} watch_usb_command_t;

static char s_command[WATCH_USB_DIAGNOSTIC_COMMAND_SIZE];
static size_t s_command_length;
static bool s_command_overflow;

static void send_text(const char *text)
{
    watch_usb_cdc_write((const uint8_t *)text, strlen(text));
}

static void send_diag(void)
{
    char response[192];
    watch_diagnostic_capsule_t capsule;
    int length;

    if (!watch_diagnostic_get(&capsule)) {
        send_text("diag=none\r\n");
        return;
    }

    length = snprintf(response, sizeof(response),
                      "diag reason=%lu count=%lu pc=0x%08lx cfsr=0x%08lx\r\n",
                      (unsigned long)capsule.reason, (unsigned long)capsule.count,
                      (unsigned long)capsule.pc, (unsigned long)capsule.cfsr);
    if ((length > 0) && ((size_t)length < sizeof(response))) {
        watch_usb_cdc_write((const uint8_t *)response, (size_t)length);
    }
}

static void send_stats(void)
{
    char response[128];
    int length = snprintf(response, sizeof(response),
                          "stats rx_pending=%lu rx_drop=%lu tx_drop=%lu\r\n",
                          (unsigned long)watch_usb_cdc_rx_pending(),
                          (unsigned long)watch_usb_cdc_rx_dropped(),
                          (unsigned long)watch_usb_cdc_tx_dropped());
    if ((length > 0) && ((size_t)length < sizeof(response))) {
        watch_usb_cdc_write((const uint8_t *)response, (size_t)length);
    }
}

static const char *health_state_name(watch_runtime_health_state_t state)
{
    switch (state) {
    case WATCH_RUNTIME_HEALTH_NOT_STARTED:
        return "new";
    case WATCH_RUNTIME_HEALTH_HEALTHY:
        return "ok";
    case WATCH_RUNTIME_HEALTH_STALE:
        return "stale";
    case WATCH_RUNTIME_HEALTH_COUNT:
        return "invalid";
    }

    return "invalid";
}

static void send_health(void)
{
    char response[224];
    watch_runtime_health_t app_health;
    watch_runtime_health_t ui_health;
    watch_runtime_health_t usb_health;
    uint32_t now_ms = HAL_GetTick();
    int length;

    if (!watch_runtime_read_health(WATCH_RUNTIME_SERVICE_APP, now_ms, &app_health)
        || !watch_runtime_read_health(WATCH_RUNTIME_SERVICE_UI, now_ms, &ui_health)
        || !watch_runtime_read_health(WATCH_RUNTIME_SERVICE_USB, now_ms, &usb_health)) {
        send_text("health=unavailable\r\n");
        return;
    }

    length = snprintf(response, sizeof(response),
                      "health stage=%u app=%s/%lu ui=%s/%lu usb=%s/%lu queue=%u\r\n",
                      (unsigned int)watch_runtime_init_stage(), health_state_name(app_health.state),
                      (unsigned long)app_health.heartbeat_count, health_state_name(ui_health.state),
                      (unsigned long)ui_health.heartbeat_count, health_state_name(usb_health.state),
                      (unsigned long)usb_health.heartbeat_count,
                      (unsigned int)watch_runtime_service_event_count());
    if ((length > 0) && ((size_t)length < sizeof(response))) {
        watch_usb_cdc_write((const uint8_t *)response, (size_t)length);
    }
}

static void handle_command(watch_usb_command_t command)
{
    switch (command) {
    case WATCH_USB_COMMAND_HELP:
        send_text("commands: help ping info diag stats health\r\n");
        break;
    case WATCH_USB_COMMAND_PING:
        send_text("pong\r\n");
        break;
    case WATCH_USB_COMMAND_INFO:
        send_text("watch=f411 usb=cdc protocol=1 display=240x280\r\n");
        break;
    case WATCH_USB_COMMAND_DIAG:
        send_diag();
        break;
    case WATCH_USB_COMMAND_STATS:
        send_stats();
        break;
    case WATCH_USB_COMMAND_HEALTH:
        send_health();
        break;
    case WATCH_USB_COMMAND_UNKNOWN:
    case WATCH_USB_COMMAND_COUNT:
        send_text("error=unknown-command\r\n");
        break;
    }
}

static watch_usb_command_t parse_command(void)
{
    s_command[s_command_length] = '\0';

    if (strcmp(s_command, "help") == 0) {
        return WATCH_USB_COMMAND_HELP;
    } else if (strcmp(s_command, "ping") == 0) {
        return WATCH_USB_COMMAND_PING;
    } else if (strcmp(s_command, "info") == 0) {
        return WATCH_USB_COMMAND_INFO;
    } else if (strcmp(s_command, "diag") == 0) {
        return WATCH_USB_COMMAND_DIAG;
    } else if (strcmp(s_command, "stats") == 0) {
        return WATCH_USB_COMMAND_STATS;
    } else if (strcmp(s_command, "health") == 0) {
        return WATCH_USB_COMMAND_HEALTH;
    }

    return WATCH_USB_COMMAND_UNKNOWN;
}

static void consume_byte(uint8_t byte)
{
    if ((byte == '\r') || (byte == '\n')) {
        watch_service_event_t event;

        if (s_command_overflow) {
            send_text("error=line-too-long\r\n");
        } else if (s_command_length > 0U) {
            event = (watch_service_event_t) {
                .type = WATCH_USB_DIAGNOSTIC_EVENT_COMMAND,
                .value = (uint32_t)parse_command(),
                .timestamp_ms = HAL_GetTick(),
            };
            if (!watch_runtime_post_service_event(&event)) {
                send_text("error=service-queue-full\r\n");
            }
        }

        s_command_length = 0U;
        s_command_overflow = false;
        return;
    }

    if (s_command_overflow) {
        return;
    }

    if (s_command_length < (sizeof(s_command) - 1U)) {
        s_command[s_command_length] = (char)byte;
        ++s_command_length;
    } else {
        s_command_overflow = true;
    }
}

static void process_service_events(void)
{
    watch_service_event_t event;

    while (watch_runtime_take_service_event(&event)) {
        if (event.type == WATCH_USB_DIAGNOSTIC_EVENT_COMMAND
            && event.value >= WATCH_USB_COMMAND_HELP && event.value < WATCH_USB_COMMAND_COUNT) {
            handle_command((watch_usb_command_t)event.value);
        } else {
            send_text("error=unknown-service-event\r\n");
        }
    }
}

void watch_usb_diagnostic_process(void)
{
    uint8_t input[WATCH_USB_DIAGNOSTIC_READ_SIZE];
    size_t length = watch_usb_cdc_read(input, sizeof(input));
    uint32_t now_ms = HAL_GetTick();

    (void)watch_runtime_start_service(WATCH_RUNTIME_SERVICE_USB, now_ms);
    (void)watch_runtime_heartbeat(WATCH_RUNTIME_SERVICE_USB, now_ms);

    if (watch_app_is_ready()) {
        (void)watch_ui_start();
    }

    for (size_t index = 0U; index < length; ++index) {
        consume_byte(input[index]);
    }

    process_service_events();
    watch_usb_cdc_process();
}
