#ifndef WATCH_UI_SIMULATOR_LV_CONF_H
#define WATCH_UI_SIMULATOR_LV_CONF_H

/* Keep host widget and memory switches identical to the accepted F411 port. */
#include "../../../firmware/stm32/f411_watch/user/ui/lv_conf.h"

/* The host smoke path creates every V1 page in one lifecycle run. Keep the
 * firmware contract at 16 KiB while giving the diagnostic simulator enough
 * heap to exercise repeated page construction. */
#undef LV_MEM_SIZE
#define LV_MEM_SIZE (64U * 1024U)

#if defined(_WIN32)
#undef LV_USE_OS
#define LV_USE_OS LV_OS_WINDOWS
#define LV_USE_WINDOWS 1
#undef LV_USE_FLEX
#define LV_USE_FLEX 1
#undef LV_USE_BUTTONMATRIX
#define LV_USE_BUTTONMATRIX 1
#undef LV_USE_KEYBOARD
#define LV_USE_KEYBOARD 1
#undef LV_USE_TEXTAREA
#define LV_USE_TEXTAREA 1
#else
#define LV_USE_WINDOWS 0
#endif

#endif /* WATCH_UI_SIMULATOR_LV_CONF_H */
