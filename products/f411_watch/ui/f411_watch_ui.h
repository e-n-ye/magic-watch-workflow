/**
 * @file f411_watch_ui.h
 * @brief Public wrapper around the manually exported Editor UI.
 */

#ifndef F411_WATCH_UI_H
#define F411_WATCH_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "f411_watch_ui_gen.h"

void f411_watch_ui_init(const char *asset_path);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* F411_WATCH_UI_H */
