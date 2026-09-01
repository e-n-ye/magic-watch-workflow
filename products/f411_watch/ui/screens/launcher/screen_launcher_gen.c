/**
 * @file screen_launcher_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "screen_launcher_gen.h"
#include "../../f411_watch_ui.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *  STATIC VARIABLES
 **********************/

/***********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * screen_launcher_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_launcher;
    static lv_style_t style_card;

    static bool style_inited = false;

    if (!style_inited) {
        /*Init all styles*/
        lv_style_init(&style_launcher);
        lv_style_init(&style_card);

        lv_style_set_bg_opa(&style_launcher, (255 * 100 / 100));
        lv_style_set_bg_color(&style_launcher, BG_BASE);
        lv_style_set_bg_opa(&style_card, (255 * 100 / 100));
        lv_style_set_bg_color(&style_card, SURFACE_CARD);
        lv_style_set_border_width(&style_card, 1);
        lv_style_set_border_color(&style_card, BORDER);
        lv_style_set_radius(&style_card, RADIUS_SM);
        lv_style_set_pad_all(&style_card, 0);

        style_inited = true;
    }


    lv_obj_t * the_root = NULL;

    #if F411_WATCH_UI_CHECK_COMPILE_TARGET(F411_WATCH_UI_TARGET_ALL)
    if (f411_watch_ui_check_target(F411_WATCH_UI_TARGET_ALL)) {
        lv_obj_t * lv_obj_0 = lv_obj_create(NULL);
        lv_obj_set_name_static(lv_obj_0, "screen_launcher_#");

        lv_obj_add_style(lv_obj_0, &style_launcher, 0);
        lv_obj_t * page_brand = lv_label_create(lv_obj_0);
        lv_obj_set_name(page_brand, "page_brand");
        lv_label_set_text(page_brand, "MAGIC WATCH");
        lv_obj_set_align(page_brand, LV_ALIGN_TOP_MID);
        lv_obj_set_y(page_brand, 10);
        lv_obj_set_width(page_brand, CONTENT_WIDTH);
        lv_obj_set_style_text_color(page_brand, TEXT_PRIMARY, 0);

        lv_obj_t * page_title = lv_label_create(lv_obj_0);
        lv_obj_set_name(page_title, "page_title");
        lv_label_set_text(page_title, "LAUNCHER");
        lv_obj_set_align(page_title, LV_ALIGN_TOP_MID);
        lv_obj_set_y(page_title, 30);
        lv_obj_set_width(page_title, CONTENT_WIDTH);
        lv_obj_set_style_text_color(page_title, ACCENT, 0);

        lv_obj_t * launcher_list = lv_obj_create(lv_obj_0);
        lv_obj_set_name(launcher_list, "launcher_list");
        lv_obj_set_x(launcher_list, SAFE_SIDE);
        lv_obj_set_y(launcher_list, 66);
        lv_obj_set_width(launcher_list, CONTENT_WIDTH);
        lv_obj_set_height(launcher_list, 176);
        lv_obj_set_flag(launcher_list, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_remove_style_all(launcher_list);
        lv_obj_t * launcher_item_status = lv_obj_create(launcher_list);
        lv_obj_set_name(launcher_item_status, "launcher_item_status");
        lv_obj_set_x(launcher_item_status, 0);
        lv_obj_set_y(launcher_item_status, 0);
        lv_obj_set_width(launcher_item_status, CONTENT_WIDTH);
        lv_obj_set_height(launcher_item_status, ROW_HEIGHT);
        lv_obj_set_flag(launcher_item_status, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_add_style(launcher_item_status, &style_card, 0);
        lv_obj_t * launcher_label_status = lv_label_create(launcher_item_status);
        lv_obj_set_name(launcher_label_status, "launcher_label_status");
        lv_label_set_text(launcher_label_status, "[ST] STATUS");
        lv_obj_set_x(launcher_label_status, 12);
        lv_obj_set_y(launcher_label_status, 10);
        lv_obj_set_width(launcher_label_status, 184);
        lv_obj_set_style_text_color(launcher_label_status, TEXT_PRIMARY, 0);

        lv_obj_t * launcher_item_timer = lv_obj_create(launcher_list);
        lv_obj_set_name(launcher_item_timer, "launcher_item_timer");
        lv_obj_set_x(launcher_item_timer, 0);
        lv_obj_set_y(launcher_item_timer, 44);
        lv_obj_set_width(launcher_item_timer, CONTENT_WIDTH);
        lv_obj_set_height(launcher_item_timer, ROW_HEIGHT);
        lv_obj_set_flag(launcher_item_timer, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_add_style(launcher_item_timer, &style_card, 0);
        lv_obj_t * launcher_label_timer = lv_label_create(launcher_item_timer);
        lv_obj_set_name(launcher_label_timer, "launcher_label_timer");
        lv_label_set_text(launcher_label_timer, "[TM] TIMER");
        lv_obj_set_x(launcher_label_timer, 12);
        lv_obj_set_y(launcher_label_timer, 10);
        lv_obj_set_width(launcher_label_timer, 184);
        lv_obj_set_style_text_color(launcher_label_timer, TEXT_PRIMARY, 0);

        lv_obj_t * launcher_item_calendar = lv_obj_create(launcher_list);
        lv_obj_set_name(launcher_item_calendar, "launcher_item_calendar");
        lv_obj_set_x(launcher_item_calendar, 0);
        lv_obj_set_y(launcher_item_calendar, 88);
        lv_obj_set_width(launcher_item_calendar, CONTENT_WIDTH);
        lv_obj_set_height(launcher_item_calendar, ROW_HEIGHT);
        lv_obj_set_flag(launcher_item_calendar, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_add_style(launcher_item_calendar, &style_card, 0);
        lv_obj_t * launcher_label_calendar = lv_label_create(launcher_item_calendar);
        lv_obj_set_name(launcher_label_calendar, "launcher_label_calendar");
        lv_label_set_text(launcher_label_calendar, "[CA] CALENDAR");
        lv_obj_set_x(launcher_label_calendar, 12);
        lv_obj_set_y(launcher_label_calendar, 10);
        lv_obj_set_width(launcher_label_calendar, 184);
        lv_obj_set_style_text_color(launcher_label_calendar, TEXT_PRIMARY, 0);

        lv_obj_t * launcher_item_settings = lv_obj_create(launcher_list);
        lv_obj_set_name(launcher_item_settings, "launcher_item_settings");
        lv_obj_set_x(launcher_item_settings, 0);
        lv_obj_set_y(launcher_item_settings, 132);
        lv_obj_set_width(launcher_item_settings, CONTENT_WIDTH);
        lv_obj_set_height(launcher_item_settings, ROW_HEIGHT);
        lv_obj_set_flag(launcher_item_settings, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_add_style(launcher_item_settings, &style_card, 0);
        lv_obj_t * launcher_label_settings = lv_label_create(launcher_item_settings);
        lv_obj_set_name(launcher_label_settings, "launcher_label_settings");
        lv_label_set_text(launcher_label_settings, "[SE] SETTINGS");
        lv_obj_set_x(launcher_label_settings, 12);
        lv_obj_set_y(launcher_label_settings, 10);
        lv_obj_set_width(launcher_label_settings, 184);
        lv_obj_set_style_text_color(launcher_label_settings, TEXT_PRIMARY, 0);

        lv_obj_t * page_hint = lv_label_create(lv_obj_0);
        lv_obj_set_name(page_hint, "page_hint");
        lv_label_set_text(page_hint, "SELECT: STATUS");
        lv_obj_set_align(page_hint, LV_ALIGN_BOTTOM_MID);
        lv_obj_set_y(page_hint, -12);
        lv_obj_set_width(page_hint, CONTENT_WIDTH);
        lv_obj_set_style_text_color(page_hint, TEXT_MUTED, 0);

        the_root = lv_obj_0;
    }
    #endif

    LV_TRACE_OBJ_CREATE("finished");

    return the_root;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

