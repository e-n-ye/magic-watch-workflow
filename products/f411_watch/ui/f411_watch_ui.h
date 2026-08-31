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

/* Current Editor export predates the font declarations in globals.xml. Keep
 * the checked-in export buildable until the Editor regenerates its font data. */
#ifndef montserrat_8
#define montserrat_8 (&lv_font_montserrat_14)
#endif
#ifndef montserrat_24
#define montserrat_24 (&lv_font_montserrat_14)
#endif
#ifndef montserrat_40
#define montserrat_40 (&lv_font_montserrat_14)
#endif

void f411_watch_ui_init(const char *asset_path);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* F411_WATCH_UI_H */
