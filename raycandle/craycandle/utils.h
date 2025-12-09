#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define CM_OFF
#include "cust_malloc.h"
#include "raycandle.h"

#include "log.h"

#define RC_DATA_IN_LIMIT(data, limit)                                          \
  (((limit).limit_min <= ((data))) && (data) <= ((limit).limit_max))

double RC_PIXEL_X_2_DATA(int pixel, Axes *axes);
#define RC_PIXEL_Y_2_DATA(pixel, axes)                                         \
  (((((axes)->height - ((pixel) - (axes)->startY)) / (double)(axes)->height) * \
    (axes)->ylocator.limit.diff) +                                             \
   (axes)->ylocator.limit.limit_min)
#define RC_DATA_X_2_PIXEL please use GetMousePosition()
#define RC_DATA_Y_2_PIXEL(data, axes)                                          \
  (((((axes)->ylocator.limit.limit_max - (data)) /                             \
     (axes)->ylocator.limit.diff) *                                            \
    (axes)->height) +                                                          \
   (axes)->startY)
#define RC_COLOR1_EQUALS_COLOR2(color1, color2)                                \
  ((color1).r == (color2).r && (color1).g == (color2).g &&                     \
   (color1).b == (color2).b && (color1).a == (color2).a)

long int maxl(long int a, long int b);
long int minl(long int a, long int b);

#define RC_ECHO(__any) #__any
#define FIGURE_FONT(__pfigure) ((__pfigure)->font)

#if RAYCANDLE_DEBUG
#include <assert.h>
#define RC_ASSERT(cond, ...) assert(cond)
#else
#define RC_ASSERT(cond, ...)                                                   \
  do {                                                                         \
    if (!(cond)) {                                                             \
      RC_ERROR("condition '" #cond "' failed ;" __VA_ARGS__);                  \
    }                                                                          \
  } while (0)
#endif



#define RC_INFO(format, ...)  log_info(format,##__VA_ARGS__)
#define RC_WARN(format, ...) log_warn(format,##__VA_ARGS__)
#define RC_ERROR(format, ...) log_error(format,##__VA_ARGS__)

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

typedef enum {
  RC_ALIGNMENT_LEFT,
  RC_ALIGNMENT_CENTER,
  RC_ALIGNMENT_RIGHT,
} Rc_Alignment;


float align_text(Font font, const char *text, int width, int font_size,int spacing,Rc_Alignment alignment); // returns startx for the text

Color FadeToBlack(Color c, float amount);

Color axes_get_next_tableau_t10_color(Axes *axes);

/*some figure defaults*/
#undef CUSM_PREFIX
