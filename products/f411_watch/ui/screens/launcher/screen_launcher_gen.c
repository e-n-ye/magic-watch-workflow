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
    static lv_style_t style_list;
    static lv_style_t style_card;
    static lv_style_t style_card_selected;

    static bool style_inited = false;

    if (!style_inited) {
        /*Init all styles*/
        lv_style_init(&style_launcher);
        lv_style_init(&style_list);
        lv_style_init(&style_card);
        lv_style_init(&style_card_selected);

        lv_style_set_bg_opa(&style_launcher, (255 * 100 / 100));
        lv_style_set_bg_color(&style_launcher, BG_BASE);
        lv_style_set_bg_opa(&style_list, (255 * 0 / 100));
        lv_style_set_border_width(&style_list, 0);
        lv_style_set_radius(&style_list, 0);
        lv_style_set_pad_all(&style_list, 0);
        lv_style_set_bg_opa(&style_card, (255 * 100 / 100));
        lv_style_set_bg_color(&style_card, SURFACE_CARD);
        lv_style_set_border_width(&style_card, 1);
        lv_style_set_border_color(&style_card, BORDER);
        lv_style_set_radius(&style_card, RADIUS_SM);
        lv_style_set_pad_all(&style_card, 0);
        lv_style_set_bg_opa(&style_card_selected, (255 * 100 / 100));
        lv_style_set_bg_color(&style_card_selected, SURFACE_SELECTED);
        lv_style_set_border_width(&style_card_selected, 2);
        lv_style_set_border_color(&style_card_selected, ACCENT);
        lv_style_set_radius(&style_card_selected, RADIUS_SM);
        lv_style_set_pad_all(&style_card_selected, 0);

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
        lv_obj_set_x(page_brand, 16);
        lv_obj_set_y(page_brand, 8);
        lv_obj_set_width(page_brand, CONTENT_WIDTH);
        lv_obj_set_height(page_brand, 18);
        lv_obj_set_style_text_color(page_brand, TEXT_PRIMARY, 0);
        lv_obj_set_style_text_align(page_brand, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_flag(page_brand, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * page_title = lv_label_create(lv_obj_0);
        lv_obj_set_name(page_title, "page_title");
        lv_label_set_text(page_title, "LAUNCHER");
        lv_obj_set_x(page_title, 16);
        lv_obj_set_y(page_title, 28);
        lv_obj_set_width(page_title, CONTENT_WIDTH);
        lv_obj_set_height(page_title, 18);
        lv_obj_set_style_text_color(page_title, ACCENT, 0);
        lv_obj_set_style_text_align(page_title, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_flag(page_title, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * launcher_list = lv_obj_create(lv_obj_0);
        lv_obj_set_name(launcher_list, "launcher_list");
        lv_obj_set_x(launcher_list, 16);
        lv_obj_set_y(launcher_list, 58);
        lv_obj_set_width(launcher_list, 208);
        lv_obj_set_height(launcher_list, 178);
        lv_obj_set_flag(launcher_list, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_set_flag(launcher_list, LV_OBJ_FLAG_IGNORE_LAYOUT, true);
        lv_obj_add_style(launcher_list, &style_list, 0);
        lv_obj_t * launcher_item_status = lv_obj_create(launcher_list);
        lv_obj_set_name(launcher_item_status, "launcher_item_status");
        lv_obj_set_x(launcher_item_status, 0);
        lv_obj_set_y(launcher_item_status, 0);
        lv_obj_set_width(launcher_item_status, 208);
        lv_obj_set_height(launcher_item_status, 40);
        lv_obj_set_flag(launcher_item_status, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_set_flag(launcher_item_status, LV_OBJ_FLAG_CLICKABLE, true);
        lv_obj_add_style(launcher_item_status, &style_card, 0);
        lv_obj_t * launcher_icon_status = lv_image_create(launcher_item_status);
        lv_obj_set_name(launcher_icon_status, "launcher_icon_status");
        lv_image_set_src(launcher_icon_status, icon_status);
        lv_obj_set_x(launcher_icon_status, 12);
        lv_obj_set_y(launcher_icon_status, 12);
        lv_obj_set_width(launcher_icon_status, 16);
        lv_obj_set_height(launcher_icon_status, 16);
        lv_obj_set_flag(launcher_icon_status, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * launcher_label_status = lv_label_create(launcher_item_status);
        lv_obj_set_name(launcher_label_status, "launcher_label_status");
        lv_label_set_text(launcher_label_status, "STATUS");
        lv_obj_set_x(launcher_label_status, 40);
        lv_obj_set_y(launcher_label_status, 10);
        lv_obj_set_width(launcher_label_status, 150);
        lv_obj_set_height(launcher_label_status, 18);
        lv_obj_set_style_text_color(launcher_label_status, TEXT_PRIMARY, 0);
        lv_obj_set_flag(launcher_label_status, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * launcher_item_timer = lv_obj_create(launcher_list);
        lv_obj_set_name(launcher_item_timer, "launcher_item_timer");
        lv_obj_set_x(launcher_item_timer, 0);
        lv_obj_set_y(launcher_item_timer, 44);
        lv_obj_set_width(launcher_item_timer, 208);
        lv_obj_set_height(launcher_item_timer, 40);
        lv_obj_set_flag(launcher_item_timer, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_set_flag(launcher_item_timer, LV_OBJ_FLAG_CLICKABLE, true);
        lv_obj_add_style(launcher_item_timer, &style_card, 0);
        lv_obj_t * launcher_icon_timer = lv_image_create(launcher_item_timer);
        lv_obj_set_name(launcher_icon_timer, "launcher_icon_timer");
        lv_image_set_src(launcher_icon_timer, icon_timer);
        lv_obj_set_x(launcher_icon_timer, 12);
        lv_obj_set_y(launcher_icon_timer, 12);
        lv_obj_set_width(launcher_icon_timer, 16);
        lv_obj_set_height(launcher_icon_timer, 16);
        lv_obj_set_flag(launcher_icon_timer, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * launcher_label_timer = lv_label_create(launcher_item_timer);
        lv_obj_set_name(launcher_label_timer, "launcher_label_timer");
        lv_label_set_text(launcher_label_timer, "TIMER");
        lv_obj_set_x(launcher_label_timer, 40);
        lv_obj_set_y(launcher_label_timer, 10);
        lv_obj_set_width(launcher_label_timer, 150);
        lv_obj_set_height(launcher_label_timer, 18);
        lv_obj_set_style_text_color(launcher_label_timer, TEXT_PRIMARY, 0);
        lv_obj_set_flag(launcher_label_timer, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * launcher_item_calendar = lv_obj_create(launcher_list);
        lv_obj_set_name(launcher_item_calendar, "launcher_item_calendar");
        lv_obj_set_x(launcher_item_calendar, 0);
        lv_obj_set_y(launcher_item_calendar, 88);
        lv_obj_set_width(launcher_item_calendar, 208);
        lv_obj_set_height(launcher_item_calendar, 40);
        lv_obj_set_flag(launcher_item_calendar, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_set_flag(launcher_item_calendar, LV_OBJ_FLAG_CLICKABLE, true);
        lv_obj_add_style(launcher_item_calendar, &style_card, 0);
        lv_obj_t * launcher_icon_calendar = lv_image_create(launcher_item_calendar);
        lv_obj_set_name(launcher_icon_calendar, "launcher_icon_calendar");
        lv_image_set_src(launcher_icon_calendar, icon_calendar);
        lv_obj_set_x(launcher_icon_calendar, 12);
        lv_obj_set_y(launcher_icon_calendar, 12);
        lv_obj_set_width(launcher_icon_calendar, 16);
        lv_obj_set_height(launcher_icon_calendar, 16);
        lv_obj_set_flag(launcher_icon_calendar, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * launcher_label_calendar = lv_label_create(launcher_item_calendar);
        lv_obj_set_name(launcher_label_calendar, "launcher_label_calendar");
        lv_label_set_text(launcher_label_calendar, "CALENDAR");
        lv_obj_set_x(launcher_label_calendar, 40);
        lv_obj_set_y(launcher_label_calendar, 10);
        lv_obj_set_width(launcher_label_calendar, 150);
        lv_obj_set_height(launcher_label_calendar, 18);
        lv_obj_set_style_text_color(launcher_label_calendar, TEXT_PRIMARY, 0);
        lv_obj_set_flag(launcher_label_calendar, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * launcher_item_settings = lv_obj_create(launcher_list);
        lv_obj_set_name(launcher_item_settings, "launcher_item_settings");
        lv_obj_set_x(launcher_item_settings, 0);
        lv_obj_set_y(launcher_item_settings, 132);
        lv_obj_set_width(launcher_item_settings, 208);
        lv_obj_set_height(launcher_item_settings, 40);
        lv_obj_set_flag(launcher_item_settings, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_set_flag(launcher_item_settings, LV_OBJ_FLAG_CLICKABLE, true);
        lv_obj_add_style(launcher_item_settings, &style_card, 0);
        lv_obj_t * launcher_icon_settings = lv_image_create(launcher_item_settings);
        lv_obj_set_name(launcher_icon_settings, "launcher_icon_settings");
        lv_image_set_src(launcher_icon_settings, icon_settings);
        lv_obj_set_x(launcher_icon_settings, 12);
        lv_obj_set_y(launcher_icon_settings, 12);
        lv_obj_set_width(launcher_icon_settings, 16);
        lv_obj_set_height(launcher_icon_settings, 16);
        lv_obj_set_flag(launcher_icon_settings, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * launcher_label_settings = lv_label_create(launcher_item_settings);
        lv_obj_set_name(launcher_label_settings, "launcher_label_settings");
        lv_label_set_text(launcher_label_settings, "SETTINGS");
        lv_obj_set_x(launcher_label_settings, 40);
        lv_obj_set_y(launcher_label_settings, 10);
        lv_obj_set_width(launcher_label_settings, 150);
        lv_obj_set_height(launcher_label_settings, 18);
        lv_obj_set_style_text_color(launcher_label_settings, TEXT_PRIMARY, 0);
        lv_obj_set_flag(launcher_label_settings, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * page_hint = lv_label_create(lv_obj_0);
        lv_obj_set_name(page_hint, "page_hint");
        lv_label_set_text(page_hint, "TOUCH A CARD TO OPEN");
        lv_obj_set_x(page_hint, 16);
        lv_obj_set_y(page_hint, 262);
        lv_obj_set_width(page_hint, CONTENT_WIDTH);
        lv_obj_set_height(page_hint, 8);
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

