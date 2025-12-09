#include <stdlib.h>

#include "raycandle.h"
#define CS_IMPLEMENTATION
#define CM_SILENT !RAYCANDLE_DEBUG
#include "cs_string.h"

#define CM_SILENT !RAYCANDLE_DEBUG
#define CM_IMPLEMENTATION
#include "cust_malloc.h"

#define LOG_IMPLEMENTATION
#include "log.h"


#include "utils.h"



long int maxl(long int a, long int b) { return (a > b) ? a : b; }

long int minl(long int a, long int b) { return (a < b) ? a : b; }

Color FadeToBlack(Color c, float amount) {
  Color out;
  out.r = c.r * (1 - amount);
  out.g = c.g * (1 - amount);
  out.b = c.b * (1 - amount);
  out.a = c.a;
  return out;
}


float align_text(Font font, const char *text, int width, int font_size,
                 int spacing, Rc_Alignment alignment) {
  float f = MeasureTextEx(font, text, font_size, spacing).x;
  if (f == 0.f) {
    f = MeasureText(text, font_size);
  }
  if ((float)width < f) {
    return 0.f;
  }
  switch (alignment) {
  case RC_ALIGNMENT_CENTER:
    return (width - f) / 2;
  case RC_ALIGNMENT_LEFT:
    return 0.f;
  case RC_ALIGNMENT_RIGHT:
    return width - f;
  }
  RC_ERROR("non reachable condition\n");
  return 0.f;
}

Color RC_faded_color_from(Color original) {
  uint8_t fade_amount = 70;
  Color faded;
  faded.a =
      (original.a > fade_amount) ? (original.a - fade_amount) : fade_amount;
  faded.r =
      (original.r > fade_amount) ? (original.r - fade_amount) : fade_amount;
  faded.g =
      (original.g > fade_amount) ? (original.g - fade_amount) : fade_amount;
  faded.b =
      (original.b > fade_amount) ? (original.b - fade_amount) : fade_amount;
  return faded;
}

Color axes_get_next_tableau_t10_color(Axes *axes) {
  RC_ASSERT(axes->tableau_t10_index >= 0 && axes->tableau_t10_index < 10);
  Color color = tableau_colors[(axes->tableau_t10_index++)];
  axes->tableau_t10_index %= 10;
  return color;
}

double RC_PIXEL_X_2_DATA(int pixel, Axes *axes) {
  double a = ((pixel - axes->startX) / (double)axes->width) *
                 axes->parent->dragger.vlen +
             axes->parent->dragger.start;
  return axes->parent->dragger.xdata[(size_t)a] +
         (a - ((long int)a)) * axes->parent->dragger.timeframe;
}
