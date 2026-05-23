#ifndef __RAYCANDLE_H__
#define __RAYCANDLE_H__

/* #include "axes.h" */
/* #include "artist.h" */
#include "node.h"
#include "raylib.h"

typedef char*  Str;
typedef enum {
  LEGEND_POSITION_NO_LEGEND = 0,
  LEGEND_POSITION_TOP_LEFT = 1,
  LEGEND_POSITION_TOP_RIGHT = 2,
  LEGEND_POSITION_BOTTOM_LEFT = 3,
  LEGEND_POSITION_BOTTOM_RIGHT = 4,
} LegendPosition;

typedef struct Figure Figure;
typedef struct {
  Str str;
  LegendPosition lp;
}Legend;


// Color port from raylib to cffi
typedef struct {
  unsigned char r,g,b,a;
}RColor;
#define RColor Color


typedef struct {
  float min,max;
}Limit;


typedef struct Artist Artist;
typedef enum ArtistType ArtistType;
typedef struct ArtistVTable ArtistVTable;

enum  ArtistType  {
  ARTIST_TYPE_LINE = 0,
  ARTIST_TYPE_CANDLE = 1,
};

/*
  if is_splined then data len is that set by figure_set_view_data
  else:data len is double[4]
    if is_relative then data is double[4] with {startx,endx, starty, endy} ranging from (0.f,1.f). e.g 0,1,0.5,0.5 will draw horizontal line
    else normal limits will be applied
 */
typedef struct {
  bool is_splined;
  bool is_relative;
  double* data;
}LineData;


struct Artist{
  ArtistType atype;
  void *data;
  Artist* next;
  RColor* colors;
  ArtistVTable* avt;
  char *label;
  float thick;
  bool is_limitable;
};

typedef struct Axes Axes;
struct Axes{
  Rect* area;
  float w;
  size_t ylabel_len, ylabel_padding;
  Figure *fig;
  Artist *artist;
  size_t artist_len;
  Legend legend;
  RColor facecolor;
  Limit lim;
  Str title;
  uint8_t tableau_t10_index;
};

typedef struct Theme Theme;
struct Theme{
  RColor bg,fg;
  size_t font_size;
  size_t font_spacing;
  float spacing;
  bool show_title;
  char* font_path;
}; //visible config for the figure


typedef struct Fconfig Fconfig;
typedef struct Figure Figure;
struct Figure {
  Arena* arena;
  char* title;
  Theme theme;
  bool force_update;
  Fconfig* fc;
  Axes* axes;
  Limit xlim;

};

void figure_set_theme(Figure* figure, Theme theme);
void figure_set_view_data(Figure* figure, size_t len, char *xformat, double *xdata); //set view data. len is required.

double scale_to_limitf(double val, Limit src, Limit dest);
void scale_to_limit(const double *data, double *dest, size_t count, Limit srcl, Limit destl);
Limit get_lim(const double *data, size_t len);


#endif //__RAYCANDLE_H__
