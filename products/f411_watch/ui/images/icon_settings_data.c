
#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#include "lvgl.h"
#elif defined(LV_LVGL_H_INCLUDE_SYSTEM)
#include <lvgl.h>
#elif defined(LV_BUILD_TEST)
#include "../lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_ICON_SETTINGS_DATA
#define LV_ATTRIBUTE_ICON_SETTINGS_DATA
#endif

static const
LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_ICON_SETTINGS_DATA
uint8_t icon_settings_data_map[] = {

    0x00,0x00,0x00,0x00,0xfa,0xf7,0xf4,0xff,

    0x00,0x00,
    0x01,0x80,
    0x0d,0xb0,
    0x06,0x60,
    0x1b,0xd8,
    0x13,0xc8,
    0x33,0xcc,
    0x33,0xcc,
    0x13,0xc8,
    0x1b,0xd8,
    0x06,0x60,
    0x0d,0xb0,
    0x01,0x80,
    0x00,0x00,
    0x00,0x00,
    0x00,0x00,

};

const lv_image_dsc_t icon_settings_data = {
  .header = {
    .magic = LV_IMAGE_HEADER_MAGIC,
    .cf = LV_COLOR_FORMAT_I1,
    .flags = 0,
    .w = 16,
    .h = 16,
    .stride = 2,
    .reserved_2 = 0,
  },
  .data_size = sizeof(icon_settings_data_map),
  .data = icon_settings_data_map,
  .reserved = NULL,
};

