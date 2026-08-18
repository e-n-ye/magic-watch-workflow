#ifndef WATCH_LITTLEFS_H
#define WATCH_LITTLEFS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lfs.h"
#include "watch_w25_partitions.h"

#define WATCH_LITTLEFS_READ_SIZE 16U
#define WATCH_LITTLEFS_PROGRAM_SIZE WATCH_W25Q128_PAGE_SIZE
#define WATCH_LITTLEFS_BLOCK_SIZE WATCH_W25Q128_SECTOR_SIZE
#define WATCH_LITTLEFS_BLOCK_COUNT (WATCH_W25_LITTLEFS_SIZE / WATCH_LITTLEFS_BLOCK_SIZE)
#define WATCH_LITTLEFS_CACHE_SIZE WATCH_W25Q128_PAGE_SIZE
#define WATCH_LITTLEFS_LOOKAHEAD_SIZE 32U
#define WATCH_LITTLEFS_RESOURCE_CHUNK_SIZE WATCH_W25Q128_PAGE_SIZE
#define WATCH_LITTLEFS_UPLOAD_PATH_SIZE 160U

typedef bool (*watch_littlefs_read_fn)(void *context, uint32_t address, uint8_t *data,
                                       size_t length);
typedef bool (*watch_littlefs_program_fn)(void *context, uint32_t address, const uint8_t *data,
                                          size_t length);
typedef bool (*watch_littlefs_erase_fn)(void *context, uint32_t address);
typedef bool (*watch_littlefs_sync_fn)(void *context);
typedef bool (*watch_littlefs_chunk_fn)(void *context, const uint8_t *data, size_t length);

typedef struct
{
    watch_littlefs_read_fn read;
    watch_littlefs_program_fn program;
    watch_littlefs_erase_fn erase;
    watch_littlefs_sync_fn sync;
    void *context;
} watch_littlefs_backend_t;

typedef enum {
    WATCH_LITTLEFS_RESULT_OK = 0,
    WATCH_LITTLEFS_RESULT_INVALID_ARGUMENT,
    WATCH_LITTLEFS_RESULT_NOT_MOUNTED,
    WATCH_LITTLEFS_RESULT_IO,
    WATCH_LITTLEFS_RESULT_CORRUPT,
    WATCH_LITTLEFS_RESULT_NOT_FOUND,
    WATCH_LITTLEFS_RESULT_NO_SPACE,
    WATCH_LITTLEFS_RESULT_CALLBACK_REJECTED,
    WATCH_LITTLEFS_RESULT_COUNT
} watch_littlefs_result_t;

typedef struct
{
    watch_littlefs_backend_t backend;
    lfs_t filesystem;
    struct lfs_config config;
    bool initialized;
    bool mounted;
    uint8_t read_cache[WATCH_LITTLEFS_CACHE_SIZE];
    uint8_t program_cache[WATCH_LITTLEFS_CACHE_SIZE];
    uint8_t lookahead[WATCH_LITTLEFS_LOOKAHEAD_SIZE];
} watch_littlefs_t;

typedef struct
{
    watch_littlefs_t *filesystem;
    lfs_file_t file;
    bool active;
    char temporary_path[WATCH_LITTLEFS_UPLOAD_PATH_SIZE];
    char final_path[WATCH_LITTLEFS_UPLOAD_PATH_SIZE];
    size_t written;
} watch_littlefs_upload_t;

bool watch_littlefs_init(watch_littlefs_t *filesystem, const watch_littlefs_backend_t *backend);
watch_littlefs_result_t watch_littlefs_mount(watch_littlefs_t *filesystem);
watch_littlefs_result_t watch_littlefs_format(watch_littlefs_t *filesystem);
watch_littlefs_result_t watch_littlefs_unmount(watch_littlefs_t *filesystem);
watch_littlefs_result_t watch_littlefs_write_file(watch_littlefs_t *filesystem, const char *path,
                                                  const uint8_t *data, size_t length);
watch_littlefs_result_t watch_littlefs_read_file_chunks(watch_littlefs_t *filesystem,
                                                        const char *path, uint8_t *buffer,
                                                        size_t buffer_size,
                                                        watch_littlefs_chunk_fn on_chunk,
                                                        void *context, size_t *total_length);
watch_littlefs_result_t watch_littlefs_upload_begin(watch_littlefs_t *filesystem,
                                                    watch_littlefs_upload_t *upload,
                                                    const char *path);
watch_littlefs_result_t watch_littlefs_upload_write(watch_littlefs_upload_t *upload, size_t offset,
                                                    const uint8_t *data, size_t length);
watch_littlefs_result_t watch_littlefs_upload_commit(watch_littlefs_upload_t *upload);
void watch_littlefs_upload_abort(watch_littlefs_upload_t *upload);
bool watch_littlefs_is_mounted(const watch_littlefs_t *filesystem);
const char *watch_littlefs_result_name(watch_littlefs_result_t result);

#endif /* WATCH_LITTLEFS_H */
