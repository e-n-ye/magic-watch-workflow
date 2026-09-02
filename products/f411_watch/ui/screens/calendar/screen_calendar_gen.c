/**
 * @file screen_calendar_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "screen_calendar_gen.h"
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

lv_obj_t * screen_calendar_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_calendar;
    static lv_style_t style_control;

    static bool style_inited = false;

    if (!style_inited) {
        /*Init all styles*/
        lv_style_init(&style_calendar);
        lv_style_init(&style_control);

        lv_style_set_bg_opa(&style_calendar, (255 * 100 / 100));
        lv_style_set_bg_color(&style_calendar, BG_BASE);
        lv_style_set_bg_opa(&style_control, (255 * 100 / 100));
        lv_style_set_bg_color(&style_control, SURFACE_CARD);
        lv_style_set_border_width(&style_control, 1);
        lv_style_set_border_color(&style_control, BORDER);
        lv_style_set_radius(&style_control, RADIUS_SM);
        lv_style_set_pad_all(&style_control, 0);

        style_inited = true;
    }


    lv_obj_t * the_root = NULL;

    #if F411_WATCH_UI_CHECK_COMPILE_TARGET(F411_WATCH_UI_TARGET_ALL)
    if (f411_watch_ui_check_target(F411_WATCH_UI_TARGET_ALL)) {
        lv_obj_t * lv_obj_0 = lv_obj_create(NULL);
        lv_obj_set_name_static(lv_obj_0, "screen_calendar_#");

        lv_obj_add_style(lv_obj_0, &style_calendar, 0);
        lv_obj_t * page_brand = lv_label_create(lv_obj_0);
        lv_obj_set_name(page_brand, "page_brand");
        lv_label_set_text(page_brand, "MAGIC WATCH");
        lv_obj_set_x(page_brand, 16);
        lv_obj_set_y(page_brand, 8);
        lv_obj_set_width(page_brand, CONTENT_WIDTH);
        lv_obj_set_height(page_brand, 18);
        lv_obj_set_style_text_color(page_brand, TEXT_PRIMARY, 0);
        lv_obj_set_style_text_align(page_brand, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_flag(page_brand, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * page_title = lv_label_create(lv_obj_0);
        lv_obj_set_name(page_title, "page_title");
        lv_label_set_text(page_title, "CALENDAR");
        lv_obj_set_x(page_title, 16);
        lv_obj_set_y(page_title, 28);
        lv_obj_set_width(page_title, CONTENT_WIDTH);
        lv_obj_set_height(page_title, 18);
        lv_obj_set_style_text_color(page_title, ACCENT, 0);
        lv_obj_set_style_text_align(page_title, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_flag(page_title, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * calendar_prev = lv_obj_create(lv_obj_0);
        lv_obj_set_name(calendar_prev, "calendar_prev");
        lv_obj_set_x(calendar_prev, 16);
        lv_obj_set_y(calendar_prev, 62);
        lv_obj_set_width(calendar_prev, 40);
        lv_obj_set_height(calendar_prev, 32);
        lv_obj_set_flag(calendar_prev, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_set_flag(calendar_prev, LV_OBJ_FLAG_CLICKABLE, true);
        lv_obj_set_flag(calendar_prev, LV_OBJ_FLAG_IGNORE_LAYOUT, true);
        lv_obj_add_style(calendar_prev, &style_control, 0);
        lv_obj_t * calendar_prev_label = lv_label_create(calendar_prev);
        lv_obj_set_name(calendar_prev_label, "calendar_prev_label");
        lv_label_set_text(calendar_prev_label, "<");
        lv_obj_set_x(calendar_prev_label, 0);
        lv_obj_set_y(calendar_prev_label, 7);
        lv_obj_set_width(calendar_prev_label, 40);
        lv_obj_set_height(calendar_prev_label, 18);
        lv_obj_set_style_text_color(calendar_prev_label, TEXT_PRIMARY, 0);
        lv_obj_set_flag(calendar_prev_label, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * calendar_month = lv_label_create(lv_obj_0);
        lv_obj_set_name(calendar_month, "calendar_month");
        lv_label_set_text(calendar_month, "AUGUST 2026");
        lv_obj_set_x(calendar_month, 60);
        lv_obj_set_y(calendar_month, 70);
        lv_obj_set_width(calendar_month, 120);
        lv_obj_set_height(calendar_month, 18);
        lv_obj_set_style_text_color(calendar_month, TEXT_PRIMARY, 0);
        lv_obj_set_style_text_align(calendar_month, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_flag(calendar_month, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * calendar_next = lv_obj_create(lv_obj_0);
        lv_obj_set_name(calendar_next, "calendar_next");
        lv_obj_set_x(calendar_next, 184);
        lv_obj_set_y(calendar_next, 62);
        lv_obj_set_width(calendar_next, 40);
        lv_obj_set_height(calendar_next, 32);
        lv_obj_set_flag(calendar_next, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_set_flag(calendar_next, LV_OBJ_FLAG_CLICKABLE, true);
        lv_obj_set_flag(calendar_next, LV_OBJ_FLAG_IGNORE_LAYOUT, true);
        lv_obj_add_style(calendar_next, &style_control, 0);
        lv_obj_t * calendar_next_label = lv_label_create(calendar_next);
        lv_obj_set_name(calendar_next_label, "calendar_next_label");
        lv_label_set_text(calendar_next_label, ">");
        lv_obj_set_x(calendar_next_label, 0);
        lv_obj_set_y(calendar_next_label, 7);
        lv_obj_set_width(calendar_next_label, 40);
        lv_obj_set_height(calendar_next_label, 18);
        lv_obj_set_style_text_color(calendar_next_label, TEXT_PRIMARY, 0);
        lv_obj_set_flag(calendar_next_label, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * calendar_grid = lv_label_create(lv_obj_0);
        lv_obj_set_name(calendar_grid, "calendar_grid");
        lv_label_set_text(calendar_grid, "MO TU WE TH FR SA SU\n             1  2  3  4  5  6  7\n 8  9 10 11 12 13 14\n15 16 17 18 19 20 21\n22 23 24 25 26 27 28\n29 30 31");
        lv_obj_set_x(calendar_grid, 16);
        lv_obj_set_y(calendar_grid, 108);
        lv_obj_set_width(calendar_grid, 208);
        lv_obj_set_height(calendar_grid, 110);
        lv_obj_set_style_text_color(calendar_grid, TEXT_PRIMARY, 0);
        lv_obj_set_style_text_font(calendar_grid, montserrat_8, 0);
        lv_obj_set_style_text_align(calendar_grid, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_flag(calendar_grid, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * page_hint = lv_label_create(lv_obj_0);
        lv_obj_set_name(page_hint, "page_hint");
        lv_label_set_text(page_hint, "TOUCH PREV OR NEXT");
        lv_obj_set_x(page_hint, 16);
        lv_obj_set_y(page_hint, 262);
        lv_obj_set_width(page_hint, CONTENT_WIDTH);
        lv_obj_set_height(page_hint, 18);
        lv_obj_set_style_text_color(page_hint, TEXT_MUTED, 0);
        lv_obj_set_style_text_font(page_hint, montserrat_8, 0);
        lv_obj_set_style_text_align(page_hint, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_flag(page_hint, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        the_root = lv_obj_0;
    }
    #endif

    LV_TRACE_OBJ_CREATE("finished");

    return the_root;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

