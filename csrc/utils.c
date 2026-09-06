#define ARENA_IMPLEMENTATION
#include "arena.h"
#include <math.h>
#include "raycandle.h"
#include <time.h>

static const Color tableau_colors[10] = {
    {31, 119, 191, 255},  // Blue
    {255, 127, 14, 255},  // Orange
    {44, 160, 44, 255},   // Green
    {214, 39, 40, 255},   // Red
    {148, 103, 189, 255}, // Purple
    {140, 86, 75, 255},   // Brown
    {227, 119, 194, 255}, // Pink
    {127, 127, 127, 255}, // Gray
    {188, 189, 34, 255},  // Olive
    {23, 190, 207, 255}   // Cyan
};


double scale_to_limitf(double val, Limit src, Limit dest)
{
    if (isnan(val) || isnan(src.min) || isnan(src.max) || (src.max == src.min)) {
        return NAN; 
    }
    double normalized = (val - src.min) / (src.max - src.min);
    double converted = dest.min + (normalized * (dest.max - dest.min));
    return converted;
}


void scale_to_limit(const double *data, double *dest, size_t count, Limit srcl, Limit destl)
{  
  ASSERT(data && dest &&count);    
  double srcl_span = srcl.max - srcl.min;
  double destl_span = destl.max - destl.min;
  /* ASSERTM(srcl_span != 0.0, "limit.min == limit.max"); */
  double scale = destl_span / srcl_span;
  double offset = destl.min - (srcl.min * scale);

  for (size_t i = 0; i < count; i++) {
    if (!isnan(data[i])) {
      dest[i] = (data[i] * scale) + offset;
    } else {
      dest[i] = data[i];
    }
  }
}


Limit get_lim(const double *data, size_t len)
{
    if (!data || len == 0) {
        return (Limit){NAN, NAN};
    }

    Limit lim = {INFINITY, -INFINITY};
    for (size_t i = 0; i < len; i++) {
        double val = data[i];
        if (val < lim.min) lim.min = val;
        if (val > lim.max) lim.max = val;
    }
    return (lim.min <= lim.max) ? lim : (Limit){NAN, NAN};
}




Color axes_get_next_tableau_t10_color(Axes *axes)
{
  ASSERT(axes->tableau_t10_index < 10);
  Color color = tableau_colors[(axes->tableau_t10_index++)];
  axes->tableau_t10_index %= 10;
  return color;
}


  


float align_text(Font *font, const char *text, int width, int font_size, int spacing, Alignment alignment)
{
  float f = MeasureTextEx(*font, text, font_size, spacing).x;
  if (f == 0.f) {
    f = MeasureText(text, font_size);
  }
  if ((float)width < f) {
    return 0.f;
  }
  switch (alignment) {
  case ALIGNMENT_CENTER:
    return (width - f) / 2;
  case ALIGNMENT_LEFT:
    return 0.f;
  case ALIGNMENT_RIGHT:
    return width - f;
  }
  ray_error("unknown alignment state\n");
  return 0.f;
}

void epoch2strftime(int epoch, Str buffer, const char *format)
{
  unsigned int len_buffer;
  int written;
  ASSERT(format != NULL);
  time_t raw_time = epoch;
  if (epoch == 0) {
    time(&raw_time);
  }
  struct tm tm = *localtime(&raw_time);
  written = strftime(str_cur(buffer),
                     (len_buffer = str_rem(buffer)), format, &tm);
  if (written == 0) {
    fprintf(stderr, "strftime(%s) would exceed buffer size of %d bytes.\n",
            format, len_buffer);
    exit(1);
  }
}

void set_window_icon(char *file_path)
{
  ASSERT(file_path);
  Image image=LoadImage(file_path);
  SetWindowIcon(image);
  UnloadImage(image);
}
