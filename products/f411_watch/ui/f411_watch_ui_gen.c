/**
 * @file f411_watch_ui_gen.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "f411_watch_ui_gen.h"

#if defined(LV_USE_XML) && LV_USE_XML
#endif /* LV_USE_XML */

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void check_font(lv_font_t ** font, const char * name);

/**********************
 *  STATIC VARIABLES
 **********************/

static uint32_t f411_watch_ui_target = F411_WATCH_UI_TARGET_ALL;

/*----------------
 * Translations
 *----------------*/

#ifndef LV_EDITOR_PREVIEW
    static const char * translation_languages[] = {"en", NULL};
    static const char * translation_tags[] = {"watch_title", "watch_page", "launcher_title", "status_title", "settings_title", "app_status", "app_timer", "app_calendar", "app_settings", NULL};
    static const char * translation_texts[] = {
        "MAGIC WATCH", /* watch_title */
        "WATCHFACE", /* watch_page */
        "LAUNCHER", /* launcher_title */
        "STATUS", /* status_title */
        "SETTINGS", /* settings_title */
        "STATUS", /* app_status */
        "TIMER", /* app_timer */
        "CALENDAR", /* app_calendar */
        "SETTINGS", /* app_settings */
    };
#endif

/**********************
 *  GLOBAL VARIABLES
 **********************/

/*--------------------
 *  Permanent screens
 *-------------------*/

/*----------------
 * Fonts
 *----------------*/



/*----------------
 * Images
 *----------------*/



/*----------------
 * Global styles
 *----------------*/

lv_style_t screen_base;
lv_style_t surface_base;
lv_style_t surface_selected_style;
lv_style_t text_primary_style;
lv_style_t text_muted_style;
lv_style_t text_accent;

/*----------------
 * Subjects
 *----------------*/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void f411_watch_ui_init_gen(const char * asset_path)
{

    /*----------------
     * Fonts
     *----------------*/




    /*----------------
     * Images
     *----------------*/



    /*----------------
     * Global styles
     *----------------*/

    static bool style_inited = false;

    if (!style_inited) {
        /*Init all styles*/
        lv_style_init(&screen_base);
        lv_style_init(&surface_base);
        lv_style_init(&surface_selected_style);
        lv_style_init(&text_primary_style);
        lv_style_init(&text_muted_style);
        lv_style_init(&text_accent);

        lv_style_set_bg_opa(&screen_base, (255 * 100 / 100));
        lv_style_set_bg_color(&screen_base, BG_BASE);
        lv_style_set_text_color(&screen_base, TEXT_PRIMARY);
        lv_style_set_bg_opa(&surface_base, (255 * 100 / 100));
        lv_style_set_bg_color(&surface_base, SURFACE_CARD);
        lv_style_set_border_width(&surface_base, 1);
        lv_style_set_border_color(&surface_base, BORDER);
        lv_style_set_radius(&surface_base, RADIUS_SM);
        lv_style_set_pad_all(&surface_base, 0);
        lv_style_set_bg_opa(&surface_selected_style, (255 * 100 / 100));
        lv_style_set_bg_color(&surface_selected_style, SURFACE_SELECTED);
        lv_style_set_border_width(&surface_selected_style, 2);
        lv_style_set_border_color(&surface_selected_style, ACCENT);
        lv_style_set_radius(&surface_selected_style, RADIUS_SM);
        lv_style_set_pad_all(&surface_selected_style, 0);
        lv_style_set_text_color(&text_primary_style, TEXT_PRIMARY);
        lv_style_set_text_color(&text_muted_style, TEXT_MUTED);
        lv_style_set_text_color(&text_accent, ACCENT);

        style_inited = true;
    }

    /*----------------
     * Subjects
     *----------------*/
    /*----------------
     * Translations
     *----------------*/

    #ifndef LV_EDITOR_PREVIEW
        lv_translation_add_static(translation_languages, translation_tags, translation_texts);
        lv_translation_set_language(translation_languages[0]);
    #endif

#if defined(LV_USE_XML) && LV_USE_XML
    /* Register widgets */


    /* Register fonts */

    /* Register subjects */

    /* Register callbacks */
#endif

    /* Register all the global assets so that they won't be created again when globals.xml is parsed.
     * While running in the editor skip this step to update the preview when the XML changes */
#if defined(LV_USE_XML) && LV_USE_XML && !defined(LV_EDITOR_PREVIEW)
    /* Register images */
#endif

#if defined(LV_USE_XML) && LV_USE_XML == 0
    /*--------------------
     *  Permanent screens
     *-------------------*/
    /* If XML is enabled it's assumed that the permanent screens are created
     * manually from XML using lv_xml_create() */
#endif
}

void f411_watch_ui_set_target(uint32_t target)
{
    f411_watch_ui_target = target;
}

uint32_t f411_watch_ui_get_target(void)
{
    return f411_watch_ui_target;
}

bool f411_watch_ui_check_target(uint32_t target)
{
    return (f411_watch_ui_target & target) ? true : false;
}

/* Callbacks */

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void check_font(lv_font_t ** font, const char * name)
{
    if (!(*font)) {
        *font = (lv_font_t *)LV_FONT_DEFAULT;
        LV_LOG_WARN("font `%s` was not set. Using `LV_FONT_DEFAULT` instead", name);
    }
}