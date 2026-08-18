#ifndef WATCH_W25_PARTITIONS_H
#define WATCH_W25_PARTITIONS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "watch_w25q128.h"

#define WATCH_W25_METADATA_OFFSET 0x000000UL
#define WATCH_W25_METADATA_SIZE 0x010000UL
#define WATCH_W25_CANDIDATE_OFFSET 0x010000UL
#define WATCH_W25_CANDIDATE_SIZE 0x080000UL
#define WATCH_W25_ROLLBACK_OFFSET 0x090000UL
#define WATCH_W25_ROLLBACK_SIZE 0x080000UL
#define WATCH_W25_LITTLEFS_OFFSET 0x110000UL
#define WATCH_W25_LITTLEFS_SIZE (WATCH_W25Q128_CAPACITY_BYTES - WATCH_W25_LITTLEFS_OFFSET)

#define WATCH_W25_METADATA_END (WATCH_W25_METADATA_OFFSET + WATCH_W25_METADATA_SIZE - 1U)
#define WATCH_W25_CANDIDATE_END (WATCH_W25_CANDIDATE_OFFSET + WATCH_W25_CANDIDATE_SIZE - 1U)
#define WATCH_W25_ROLLBACK_END (WATCH_W25_ROLLBACK_OFFSET + WATCH_W25_ROLLBACK_SIZE - 1U)
#define WATCH_W25_LITTLEFS_END (WATCH_W25Q128_CAPACITY_BYTES - 1U)

_Static_assert(WATCH_W25_METADATA_OFFSET == 0U, "metadata must start at flash offset zero");
_Static_assert(WATCH_W25_METADATA_OFFSET + WATCH_W25_METADATA_SIZE == WATCH_W25_CANDIDATE_OFFSET,
               "metadata and candidate partitions must be adjacent");
_Static_assert(WATCH_W25_CANDIDATE_OFFSET + WATCH_W25_CANDIDATE_SIZE == WATCH_W25_ROLLBACK_OFFSET,
               "candidate and rollback partitions must be adjacent");
_Static_assert(WATCH_W25_ROLLBACK_OFFSET + WATCH_W25_ROLLBACK_SIZE == WATCH_W25_LITTLEFS_OFFSET,
               "rollback and littlefs partitions must be adjacent");
_Static_assert(WATCH_W25_LITTLEFS_OFFSET < WATCH_W25Q128_CAPACITY_BYTES,
               "littlefs partition must fit in W25Q128");
_Static_assert((WATCH_W25_LITTLEFS_OFFSET % WATCH_W25Q128_SECTOR_SIZE) == 0U,
               "littlefs partition must be sector aligned");
_Static_assert((WATCH_W25_LITTLEFS_SIZE % WATCH_W25Q128_SECTOR_SIZE) == 0U,
               "littlefs partition must end on a sector boundary");

static inline bool watch_w25_partition_contains(uint32_t offset, uint32_t size, uint32_t address,
                                                size_t length)
{
    uint64_t partition_end = (uint64_t)offset + size;
    uint64_t request_end = (uint64_t)address + length;

    return address >= offset && request_end >= address && request_end <= partition_end;
}

static inline bool watch_w25_littlefs_contains(uint32_t address, size_t length)
{
    return watch_w25_partition_contains(WATCH_W25_LITTLEFS_OFFSET, WATCH_W25_LITTLEFS_SIZE, address,
                                        length);
}

#endif /* WATCH_W25_PARTITIONS_H */
