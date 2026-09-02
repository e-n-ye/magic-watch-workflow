
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

#ifndef LV_ATTRIBUTE_ICON_STEPS_DATA
#define LV_ATTRIBUTE_ICON_STEPS_DATA
#endif

static const
LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_ICON_STEPS_DATA
uint8_t icon_steps_data_map[] = {

    0x00,0x00,0x00,0x00,0xfa,0xf7,0xf4,0xff,

    0x00,0x00,
    0x06,0x00,
    0x0f,0x00,
    0x0f,0x00,
    0x06,0x00,
    0x00,0x00,
    0x00,0x60,
    0x00,0xf0,
    0x00,0xf0,
    0x00,0x60,
    0x00,0x00,
    0x18,0x00,
    0x3c,0x00,
    0x3c,0x00,
    0x18,0x00,
    0x00,0x00,

};

const lv_image_dsc_t icon_steps_data = {
  .header = {
    .magic = LV_IMAGE_HEADER_MAGIC,
    .cf = LV_COLOR_FORMAT_I1,
    .flags = 0,
    .w = 16,
    .h = 16,
    .stride = 2,
    .reserved_2 = 0,
  },
  .data_size = sizeof(icon_steps_data_map),
  .data = icon_steps_data_map,
  .reserved = NULL,
};

