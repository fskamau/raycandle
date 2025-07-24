#include "raycandle.h"
#include "cs_string.h"
#ifndef __RAYCANDLE_LOCATOR__
#define __RAYCANDLE_LOCATOR__

typedef enum {
  LIMIT_CHANGED_XLIM,
  LIMIT_CHANGED_YLIM,
  LIMIT_CHANGED_ALL_LIM,
} LimitChanged;

void locator_update_data_buffers(Axes *axes, LimitChanged lim);
void locator_tooltip_mouse_position(Axes *axes, Str buffer,  int mouseX, int mouseY);
/**
formats `epoch` to strftime using `format` and writes formatted time into `buffer`.
Fails if formatted time length exceeds `len_buffer`
if `epoch` is 0 `epoch` is set to current epoch.
if `buffer` is NULL, formatted time is sent to stdout.
*/
void epoch2strftime(int epoch,Str buffer,const char* format);
/*
format mem into buffer accroding to format and formatter
*/
void formatter_to_str(Formatter formatter,Str format,Str buffer,void* mem);

#endif //__RAYCANDLE_LOCATOR__
