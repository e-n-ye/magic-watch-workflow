#ifndef WATCH_DIAGNOSTIC_H
#define WATCH_DIAGNOSTIC_H

#include <stdbool.h>
#include <stdint.h>

#if defined(__GNUC__)
#define WATCH_DIAGNOSTIC_NORETURN __attribute__((noreturn))
#else
#define WATCH_DIAGNOSTIC_NORETURN
#endif

#define WATCH_DIAGNOSTIC_MAGIC 0x57444350UL
#define WATCH_DIAGNOSTIC_VERSION 1U

typedef enum {
    WATCH_DIAGNOSTIC_REASON_NONE = 0U,
    WATCH_DIAGNOSTIC_REASON_NMI = 1U,
    WATCH_DIAGNOSTIC_REASON_HARDFAULT = 2U,
    WATCH_DIAGNOSTIC_REASON_MEMMANAGE = 3U,
    WATCH_DIAGNOSTIC_REASON_BUSFAULT = 4U,
    WATCH_DIAGNOSTIC_REASON_USAGEFAULT = 5U,
    WATCH_DIAGNOSTIC_REASON_ERROR = 6U,
    WATCH_DIAGNOSTIC_REASON_ASSERT = 7U,
    WATCH_DIAGNOSTIC_REASON_STACK_OVERFLOW = 8U
} watch_diagnostic_reason_t;

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t reason;
    uint32_t count;
    uint32_t cfsr;
    uint32_t hfsr;
    uint32_t dfsr;
    uint32_t mmfar;
    uint32_t bfar;
    uint32_t exc_return;
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
    uint32_t file_address;
    uint32_t line;
    uint32_t task_name_address;
    uint32_t checksum;
} watch_diagnostic_capsule_t;

void watch_diagnostic_handle_hard_fault(const uint32_t *stacked,
                                        uint32_t exc_return) WATCH_DIAGNOSTIC_NORETURN;
void watch_diagnostic_record_exception(watch_diagnostic_reason_t reason) WATCH_DIAGNOSTIC_NORETURN;
void watch_diagnostic_error(void) WATCH_DIAGNOSTIC_NORETURN;
void watch_diagnostic_assert(const char *file, uint32_t line) WATCH_DIAGNOSTIC_NORETURN;
void watch_diagnostic_stack_overflow(const signed char *task_name) WATCH_DIAGNOSTIC_NORETURN;
void watch_diagnostic_halt(void) WATCH_DIAGNOSTIC_NORETURN;

bool watch_diagnostic_get(watch_diagnostic_capsule_t *capsule);
void watch_diagnostic_clear(void);

#endif /* WATCH_DIAGNOSTIC_H */
