#include "watch_littlefs.h"

#include <string.h>

_Static_assert(WATCH_LITTLEFS_BLOCK_COUNT > 0U, "littlefs must have at least one block");
_Static_assert((WATCH_LITTLEFS_CACHE_SIZE % WATCH_LITTLEFS_READ_SIZE) == 0U,
               "cache must be a read-size multiple");
_Static_assert((WATCH_LITTLEFS_CACHE_SIZE % WATCH_LITTLEFS_PROGRAM_SIZE) == 0U,
               "cache must be a program-size multiple");

static bool watch_littlefs_backend_valid(const watch_littlefs_backend_t *backend)
{
    return backend != NULL && backend->read != NULL && backend->program != NULL
        && backend->erase != NULL && backend->sync != NULL;
}

static bool watch_littlefs_valid(const watch_littlefs_t *filesystem)
{
    return filesystem != NULL && filesystem->initialized
        && watch_littlefs_backend_valid(&filesystem->backend);
}

static bool upload_path_valid(const char *path)
{
    size_t length;

    if (path == NULL || path[0] != '/') {
        return false;
    }
    length = strlen(path);
    return length > 1U && length < WATCH_LITTLEFS_UPLOAD_PATH_SIZE && path[length - 1U] != '/';
}

static bool watch_littlefs_block_address(lfs_block_t block, lfs_off_t offset, lfs_size_t length,
                                         uint32_t *address)
{
    uint64_t partition_offset;

    if (address == NULL || block >= WATCH_LITTLEFS_BLOCK_COUNT || offset > WATCH_LITTLEFS_BLOCK_SIZE
        || length > WATCH_LITTLEFS_BLOCK_SIZE - offset) {
        return false;
    }

    partition_offset = ((uint64_t)block * WATCH_LITTLEFS_BLOCK_SIZE) + offset;
    if (partition_offset > UINT32_MAX) {
        return false;
    }

    *address = WATCH_W25_LITTLEFS_OFFSET + (uint32_t)partition_offset;
    return watch_w25_littlefs_contains(*address, length);
}

static int watch_littlefs_read_block(const struct lfs_config *config, lfs_block_t block,
                                     lfs_off_t offset, void *buffer, lfs_size_t size)
{
    watch_littlefs_t *filesystem;
    uint32_t address;

    if (config == NULL || buffer == NULL
        || !watch_littlefs_block_address(block, offset, size, &address)) {
        return LFS_ERR_IO;
    }

    filesystem = (watch_littlefs_t *)config->context;
    return watch_littlefs_valid(filesystem)
            && filesystem->backend.read(filesystem->backend.context, address, (uint8_t *)buffer,
                                        size)
        ? LFS_ERR_OK
        : LFS_ERR_IO;
}

static int watch_littlefs_program_block(const struct lfs_config *config, lfs_block_t block,
                                        lfs_off_t offset, const void *buffer, lfs_size_t size)
{
    watch_littlefs_t *filesystem;
    uint32_t address;

    if (config == NULL || buffer == NULL
        || !watch_littlefs_block_address(block, offset, size, &address)) {
        return LFS_ERR_IO;
    }

    filesystem = (watch_littlefs_t *)config->context;
    return watch_littlefs_valid(filesystem)
            && filesystem->backend.program(filesystem->backend.context, address,
                                           (const uint8_t *)buffer, size)
        ? LFS_ERR_OK
        : LFS_ERR_IO;
}

static int watch_littlefs_erase_block(const struct lfs_config *config, lfs_block_t block)
{
    watch_littlefs_t *filesystem;
    uint32_t address;

    if (config == NULL
        || !watch_littlefs_block_address(block, 0U, WATCH_LITTLEFS_BLOCK_SIZE, &address)) {
        return LFS_ERR_IO;
    }

    filesystem = (watch_littlefs_t *)config->context;
    return watch_littlefs_valid(filesystem)
            && filesystem->backend.erase(filesystem->backend.context, address)
        ? LFS_ERR_OK
        : LFS_ERR_IO;
}

static int watch_littlefs_sync_block(const struct lfs_config *config)
{
    watch_littlefs_t *filesystem;

    if (config == NULL) {
        return LFS_ERR_IO;
    }

    filesystem = (watch_littlefs_t *)config->context;
    return watch_littlefs_valid(filesystem) && filesystem->backend.sync(filesystem->backend.context)
        ? LFS_ERR_OK
        : LFS_ERR_IO;
}

static watch_littlefs_result_t watch_littlefs_result_from_lfs(int result)
{
    switch (result) {
    case LFS_ERR_OK:
        return WATCH_LITTLEFS_RESULT_OK;
    case LFS_ERR_CORRUPT:
        return WATCH_LITTLEFS_RESULT_CORRUPT;
    case LFS_ERR_NOENT:
        return WATCH_LITTLEFS_RESULT_NOT_FOUND;
    case LFS_ERR_NOSPC:
        return WATCH_LITTLEFS_RESULT_NO_SPACE;
    default:
        return WATCH_LITTLEFS_RESULT_IO;
    }
}

bool watch_littlefs_init(watch_littlefs_t *filesystem, const watch_littlefs_backend_t *backend)
{
    if (filesystem == NULL || !watch_littlefs_backend_valid(backend)) {
        return false;
    }

    memset(filesystem, 0, sizeof(*filesystem));
    filesystem->backend = *backend;
    filesystem->config = (struct lfs_config) {
        .context = filesystem,
        .read = watch_littlefs_read_block,
        .prog = watch_littlefs_program_block,
        .erase = watch_littlefs_erase_block,
        .sync = watch_littlefs_sync_block,
        .read_size = WATCH_LITTLEFS_READ_SIZE,
        .prog_size = WATCH_LITTLEFS_PROGRAM_SIZE,
        .block_size = WATCH_LITTLEFS_BLOCK_SIZE,
        .block_count = WATCH_LITTLEFS_BLOCK_COUNT,
        .block_cycles = 500,
        .cache_size = WATCH_LITTLEFS_CACHE_SIZE,
        .lookahead_size = WATCH_LITTLEFS_LOOKAHEAD_SIZE,
        .read_buffer = filesystem->read_cache,
        .prog_buffer = filesystem->program_cache,
        .lookahead_buffer = filesystem->lookahead,
    };
    filesystem->initialized = true;
    return true;
}

watch_littlefs_result_t watch_littlefs_mount(watch_littlefs_t *filesystem)
{
    int result;

    if (!watch_littlefs_valid(filesystem)) {
        return WATCH_LITTLEFS_RESULT_INVALID_ARGUMENT;
    }
    if (filesystem->mounted) {
        return WATCH_LITTLEFS_RESULT_OK;
    }

    result = lfs_mount(&filesystem->filesystem, &filesystem->config);
    if (result == LFS_ERR_OK) {
        filesystem->mounted = true;
    }
    return watch_littlefs_result_from_lfs(result);
}

watch_littlefs_result_t watch_littlefs_format(watch_littlefs_t *filesystem)
{
    int result;

    if (!watch_littlefs_valid(filesystem)) {
        return WATCH_LITTLEFS_RESULT_INVALID_ARGUMENT;
    }
    if (filesystem->mounted) {
        result = lfs_unmount(&filesystem->filesystem);
        if (result != LFS_ERR_OK) {
            return watch_littlefs_result_from_lfs(result);
        }
        filesystem->mounted = false;
    }

    result = lfs_format(&filesystem->filesystem, &filesystem->config);
    if (result != LFS_ERR_OK) {
        return watch_littlefs_result_from_lfs(result);
    }
    return watch_littlefs_mount(filesystem);
}

watch_littlefs_result_t watch_littlefs_unmount(watch_littlefs_t *filesystem)
{
    int result;

    if (!watch_littlefs_valid(filesystem)) {
        return WATCH_LITTLEFS_RESULT_INVALID_ARGUMENT;
    }
    if (!filesystem->mounted) {
        return WATCH_LITTLEFS_RESULT_OK;
    }

    result = lfs_unmount(&filesystem->filesystem);
    if (result == LFS_ERR_OK) {
        filesystem->mounted = false;
    }
    return watch_littlefs_result_from_lfs(result);
}

watch_littlefs_result_t watch_littlefs_write_file(watch_littlefs_t *filesystem, const char *path,
                                                  const uint8_t *data, size_t length)
{
    lfs_file_t file;
    watch_littlefs_result_t result = WATCH_LITTLEFS_RESULT_OK;
    int close_result;
    int open_result;
    size_t offset = 0U;

    if (!watch_littlefs_valid(filesystem) || path == NULL || (data == NULL && length > 0U)) {
        return WATCH_LITTLEFS_RESULT_INVALID_ARGUMENT;
    }
    if (!filesystem->mounted) {
        return WATCH_LITTLEFS_RESULT_NOT_MOUNTED;
    }
    open_result = lfs_file_open(&filesystem->filesystem, &file, path,
                                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    if (open_result != LFS_ERR_OK) {
        return watch_littlefs_result_from_lfs(open_result);
    }

    while (offset < length) {
        size_t remaining = length - offset;
        size_t chunk = remaining > WATCH_LITTLEFS_RESOURCE_CHUNK_SIZE
            ? WATCH_LITTLEFS_RESOURCE_CHUNK_SIZE
            : remaining;
        lfs_ssize_t written = lfs_file_write(&filesystem->filesystem, &file, &data[offset], chunk);

        if (written != (lfs_ssize_t)chunk) {
            result = watch_littlefs_result_from_lfs((int)written);
            if (written >= 0) {
                result = WATCH_LITTLEFS_RESULT_IO;
            }
            break;
        }
        offset += chunk;
    }

    close_result = lfs_file_close(&filesystem->filesystem, &file);
    if (result == WATCH_LITTLEFS_RESULT_OK && close_result != LFS_ERR_OK) {
        result = watch_littlefs_result_from_lfs(close_result);
    }
    return result;
}

watch_littlefs_result_t watch_littlefs_read_file_chunks(watch_littlefs_t *filesystem,
                                                        const char *path, uint8_t *buffer,
                                                        size_t buffer_size,
                                                        watch_littlefs_chunk_fn on_chunk,
                                                        void *context, size_t *total_length)
{
    lfs_file_t file;
    watch_littlefs_result_t result = WATCH_LITTLEFS_RESULT_OK;
    int close_result;
    int open_result;

    if (!watch_littlefs_valid(filesystem) || path == NULL || buffer == NULL || buffer_size == 0U
        || buffer_size > WATCH_LITTLEFS_RESOURCE_CHUNK_SIZE || on_chunk == NULL
        || total_length == NULL) {
        return WATCH_LITTLEFS_RESULT_INVALID_ARGUMENT;
    }
    if (!filesystem->mounted) {
        return WATCH_LITTLEFS_RESULT_NOT_MOUNTED;
    }
    open_result = lfs_file_open(&filesystem->filesystem, &file, path, LFS_O_RDONLY);
    if (open_result != LFS_ERR_OK) {
        return watch_littlefs_result_from_lfs(open_result);
    }

    *total_length = 0U;
    for (;;) {
        lfs_ssize_t read = lfs_file_read(&filesystem->filesystem, &file, buffer, buffer_size);

        if (read < 0) {
            result = watch_littlefs_result_from_lfs((int)read);
            break;
        }
        if (read == 0) {
            break;
        }
        if (!on_chunk(context, buffer, (size_t)read)) {
            result = WATCH_LITTLEFS_RESULT_CALLBACK_REJECTED;
            break;
        }
        *total_length += (size_t)read;
    }

    close_result = lfs_file_close(&filesystem->filesystem, &file);
    if (result == WATCH_LITTLEFS_RESULT_OK && close_result != LFS_ERR_OK) {
        result = watch_littlefs_result_from_lfs(close_result);
    }
    return result;
}

watch_littlefs_result_t watch_littlefs_upload_begin(watch_littlefs_t *filesystem,
                                                    watch_littlefs_upload_t *upload,
                                                    const char *path)
{
    int result;
    size_t path_length;
    size_t temporary_length;

    if (!watch_littlefs_valid(filesystem) || upload == NULL || !upload_path_valid(path)) {
        return WATCH_LITTLEFS_RESULT_INVALID_ARGUMENT;
    }
    if (!filesystem->mounted) {
        return WATCH_LITTLEFS_RESULT_NOT_MOUNTED;
    }

    memset(upload, 0, sizeof(*upload));
    path_length = strlen(path);
    temporary_length = path_length + sizeof(".upload") - 1U;
    if (temporary_length >= sizeof(upload->temporary_path)) {
        return WATCH_LITTLEFS_RESULT_INVALID_ARGUMENT;
    }
    memcpy(upload->final_path, path, path_length + 1U);
    memcpy(upload->temporary_path, path, path_length);
    memcpy(&upload->temporary_path[path_length], ".upload", sizeof(".upload"));

    (void)lfs_remove(&filesystem->filesystem, upload->temporary_path);
    result = lfs_file_open(&filesystem->filesystem, &upload->file, upload->temporary_path,
                           LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    if (result != LFS_ERR_OK) {
        return watch_littlefs_result_from_lfs(result);
    }

    upload->filesystem = filesystem;
    upload->active = true;
    return WATCH_LITTLEFS_RESULT_OK;
}

watch_littlefs_result_t watch_littlefs_upload_write(watch_littlefs_upload_t *upload, size_t offset,
                                                    const uint8_t *data, size_t length)
{
    lfs_ssize_t written;

    if (upload == NULL || !upload->active || upload->filesystem == NULL
        || (data == NULL && length > 0U) || offset != upload->written) {
        return WATCH_LITTLEFS_RESULT_INVALID_ARGUMENT;
    }
    if (length == 0U) {
        return WATCH_LITTLEFS_RESULT_OK;
    }
    written = lfs_file_write(&upload->filesystem->filesystem, &upload->file, data, length);
    if (written != (lfs_ssize_t)length) {
        return written < 0 ? watch_littlefs_result_from_lfs((int)written)
                           : WATCH_LITTLEFS_RESULT_IO;
    }
    upload->written += length;
    return WATCH_LITTLEFS_RESULT_OK;
}

watch_littlefs_result_t watch_littlefs_upload_commit(watch_littlefs_upload_t *upload)
{
    int result;

    if (upload == NULL || !upload->active || upload->filesystem == NULL) {
        return WATCH_LITTLEFS_RESULT_INVALID_ARGUMENT;
    }
    result = lfs_file_close(&upload->filesystem->filesystem, &upload->file);
    upload->active = false;
    if (result != LFS_ERR_OK) {
        (void)lfs_remove(&upload->filesystem->filesystem, upload->temporary_path);
        return watch_littlefs_result_from_lfs(result);
    }
    result =
        lfs_rename(&upload->filesystem->filesystem, upload->temporary_path, upload->final_path);
    if (result != LFS_ERR_OK) {
        (void)lfs_remove(&upload->filesystem->filesystem, upload->temporary_path);
        return watch_littlefs_result_from_lfs(result);
    }
    upload->filesystem = NULL;
    return WATCH_LITTLEFS_RESULT_OK;
}

void watch_littlefs_upload_abort(watch_littlefs_upload_t *upload)
{
    if (upload == NULL || upload->filesystem == NULL) {
        return;
    }
    if (upload->active) {
        (void)lfs_file_close(&upload->filesystem->filesystem, &upload->file);
        upload->active = false;
    }
    (void)lfs_remove(&upload->filesystem->filesystem, upload->temporary_path);
    upload->filesystem = NULL;
}

bool watch_littlefs_is_mounted(const watch_littlefs_t *filesystem)
{
    return watch_littlefs_valid(filesystem) && filesystem->mounted;
}

const char *watch_littlefs_result_name(watch_littlefs_result_t result)
{
    switch (result) {
    case WATCH_LITTLEFS_RESULT_OK:
        return "ok";
    case WATCH_LITTLEFS_RESULT_INVALID_ARGUMENT:
        return "argument";
    case WATCH_LITTLEFS_RESULT_NOT_MOUNTED:
        return "unmounted";
    case WATCH_LITTLEFS_RESULT_IO:
        return "io";
    case WATCH_LITTLEFS_RESULT_CORRUPT:
        return "corrupt";
    case WATCH_LITTLEFS_RESULT_NOT_FOUND:
        return "missing";
    case WATCH_LITTLEFS_RESULT_NO_SPACE:
        return "space";
    case WATCH_LITTLEFS_RESULT_CALLBACK_REJECTED:
        return "callback";
    case WATCH_LITTLEFS_RESULT_COUNT:
        return "invalid";
    }

    return "invalid";
}
