#include "watch_usb_diagnostic.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "board/usb/watch_usb_cdc.h"
#include "watch_app.h"
#include "watch_diagnostic.h"

#define WATCH_USB_DIAGNOSTIC_COMMAND_SIZE 96U
#define WATCH_USB_DIAGNOSTIC_READ_SIZE 32U

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

static void handle_command(void)
{
    s_command[s_command_length] = '\0';

    if (strcmp(s_command, "help") == 0) {
        send_text("commands: help ping info diag stats\r\n");
    } else if (strcmp(s_command, "ping") == 0) {
        send_text("pong\r\n");
    } else if (strcmp(s_command, "info") == 0) {
        send_text("watch=f411 usb=cdc protocol=1 display=240x280\r\n");
    } else if (strcmp(s_command, "diag") == 0) {
        send_diag();
    } else if (strcmp(s_command, "stats") == 0) {
        send_stats();
    } else {
        send_text("error=unknown-command\r\n");
    }
}

static void consume_byte(uint8_t byte)
{
    if ((byte == '\r') || (byte == '\n')) {
        if (s_command_overflow) {
            send_text("error=line-too-long\r\n");
        } else if (s_command_length > 0U) {
            handle_command();
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

void watch_usb_diagnostic_process(void)
{
    uint8_t input[WATCH_USB_DIAGNOSTIC_READ_SIZE];
    size_t length = watch_usb_cdc_read(input, sizeof(input));

    watch_app_process();

    for (size_t index = 0U; index < length; ++index) {
        consume_byte(input[index]);
    }

    watch_usb_cdc_process();
}
