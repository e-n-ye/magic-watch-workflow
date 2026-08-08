#include "watch_diagnostic.h"

#include <stddef.h>

#include "stm32f4xx.h"

static volatile watch_diagnostic_capsule_t s_capsule __attribute__((section(".noinit"), used));

_Static_assert((sizeof(watch_diagnostic_capsule_t) % sizeof(uint32_t)) == 0U,
               "diagnostic capsule must be word aligned");

static uint32_t diagnostic_checksum(const watch_diagnostic_capsule_t *capsule)
{
    const uint8_t *bytes = (const uint8_t *)capsule;
    uint32_t checksum = 2166136261UL;

    for (size_t index = 0U; index < offsetof(watch_diagnostic_capsule_t, checksum); ++index) {
        checksum ^= bytes[index];
        checksum *= 16777619UL;
    }

    return checksum;
}

static bool diagnostic_capsule_is_valid(void)
{
    if ((s_capsule.magic != WATCH_DIAGNOSTIC_MAGIC)
        || (s_capsule.version != WATCH_DIAGNOSTIC_VERSION)
        || (s_capsule.reason == WATCH_DIAGNOSTIC_REASON_NONE)) {
        return false;
    }

    return s_capsule.checksum
        == diagnostic_checksum((const watch_diagnostic_capsule_t *)&s_capsule);
}

static void diagnostic_store(const watch_diagnostic_capsule_t *capsule)
{
    const uint32_t *source = (const uint32_t *)capsule;
    volatile uint32_t *destination = (volatile uint32_t *)&s_capsule;

    for (size_t index = 0U; index < (sizeof(*capsule) / sizeof(uint32_t)); ++index) {
        destination[index] = source[index];
    }
}

static void diagnostic_record(watch_diagnostic_reason_t reason, const uint32_t *stacked,
                              uint32_t exc_return, const char *file, uint32_t line,
                              const signed char *task_name)
{
    watch_diagnostic_capsule_t capsule = { 0 };

    capsule.magic = WATCH_DIAGNOSTIC_MAGIC;
    capsule.version = WATCH_DIAGNOSTIC_VERSION;
    capsule.reason = (uint32_t)reason;
    capsule.count = diagnostic_capsule_is_valid() ? (s_capsule.count + 1U) : 1U;
    capsule.cfsr = SCB->CFSR;
    capsule.hfsr = SCB->HFSR;
    capsule.dfsr = SCB->DFSR;
    capsule.mmfar = SCB->MMFAR;
    capsule.bfar = SCB->BFAR;
    capsule.exc_return = exc_return;
    capsule.file_address = (uint32_t)(uintptr_t)file;
    capsule.line = line;
    capsule.task_name_address = (uint32_t)(uintptr_t)task_name;

    if (stacked != 0) {
        capsule.r0 = stacked[0];
        capsule.r1 = stacked[1];
        capsule.r2 = stacked[2];
        capsule.r3 = stacked[3];
        capsule.r12 = stacked[4];
        capsule.lr = stacked[5];
        capsule.pc = stacked[6];
        capsule.xpsr = stacked[7];
    }

    capsule.checksum = diagnostic_checksum(&capsule);
    diagnostic_store(&capsule);
}

void watch_diagnostic_handle_hard_fault(const uint32_t *stacked, uint32_t exc_return)
{
    diagnostic_record(WATCH_DIAGNOSTIC_REASON_HARDFAULT, stacked, exc_return, 0, 0U, 0);
    watch_diagnostic_halt();
}

void watch_diagnostic_record_exception(watch_diagnostic_reason_t reason)
{
    diagnostic_record(reason, 0, 0U, 0, 0U, 0);
    watch_diagnostic_halt();
}

void watch_diagnostic_error(void)
{
    diagnostic_record(WATCH_DIAGNOSTIC_REASON_ERROR, 0, 0U, 0, 0U, 0);
    watch_diagnostic_halt();
}

void watch_diagnostic_assert(const char *file, uint32_t line)
{
    diagnostic_record(WATCH_DIAGNOSTIC_REASON_ASSERT, 0, 0U, file, line, 0);
    watch_diagnostic_halt();
}

void watch_diagnostic_stack_overflow(const signed char *task_name)
{
    diagnostic_record(WATCH_DIAGNOSTIC_REASON_STACK_OVERFLOW, 0, 0U, 0, 0U, task_name);
    watch_diagnostic_halt();
}

void watch_diagnostic_halt(void)
{
    __disable_irq();

    while (1) {
        __NOP();
    }
}

bool watch_diagnostic_get(watch_diagnostic_capsule_t *capsule)
{
    uint32_t *destination;
    volatile const uint32_t *source;

    if ((capsule == 0) || !diagnostic_capsule_is_valid()) {
        return false;
    }

    destination = (uint32_t *)capsule;
    source = (volatile const uint32_t *)&s_capsule;
    for (size_t index = 0U; index < (sizeof(*capsule) / sizeof(uint32_t)); ++index) {
        destination[index] = source[index];
    }

    return true;
}

void watch_diagnostic_clear(void)
{
    s_capsule.magic = 0U;
    s_capsule.checksum = 0U;
}
