#ifndef WATCH_LITTLEFS_BOARD_H
#define WATCH_LITTLEFS_BOARD_H

#include <stdbool.h>
#include <stdint.h>

#include "watch_littlefs.h"

typedef struct
{
    bool initialized;
    bool mounted;
    watch_littlefs_result_t last_result;
    uint32_t image_chunks;
    uint32_t font_chunks;
    uint32_t text_chunks;
} watch_littlefs_board_status_t;

bool watch_littlefs_board_init(void);
watch_littlefs_result_t watch_littlefs_board_mount(void);
watch_littlefs_result_t watch_littlefs_board_run_resource_test(void);
bool watch_littlefs_board_read_status(watch_littlefs_board_status_t *status);

#endif /* WATCH_LITTLEFS_BOARD_H */
