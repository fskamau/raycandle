#ifndef __RAYCANDLE_H__
#define __RAYCANDLE_H__

#include "node.h"
#include "raylib.h"

/*cffi types

 */
#ifndef __NODE_H__
typedef struct {
  float x,y,w,h;
}Rect;

#endif


typedef char*  Str;
typedef enum {
  LEGEND_POSITION_NO_LEGEND = 0,
  LEGEND_POSITION_TOP_LEFT ,
  LEGEND_POSITION_TOP_RIGHT ,
  LEGEND_POSITION_BOTTOM_LEFT ,
  LEGEND_POSITION_BOTTOM_RIGHT ,
} LegendPosition;

typedef struct Figure Figure;
typedef struct {
  size_t llen;
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
typedef struct ArtistVTable ArtistVTable;

typedef enum   {
  ARTIST_TYPE_LINE = 0,
  ARTIST_TYPE_CANDLE = 1,
}ArtistType;

/*
  if is_splined then data len is that set by figure_set_view_data
  else:data len is double[4]
    if is_relative then data is double[4] with {startx,endx, starty, endy} ranging from (0.f,1.f). e.g 0,1,0.5,0.5 will draw horizontal line
    else normal limits will be applied
 */
typedef struct {
  int is_splined;
  int is_relative;
  double* data;
}LineData;

typedef struct Axes Axes;
struct Artist{
  ArtistType atype;
  void *data;
  Artist* next;
  RColor* colors;
  Axes* axes;
  ArtistVTable* avt;
  char *label;
  float thick;
  bool is_limitable;
};

typedef struct{
  Rect area;
  char *format;
}Label;

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
  size_t tableau_t10_index;
  Label ylabel;
};

typedef struct Theme Theme;
struct Theme{
  RColor bg,fg;
  size_t font_size;
  size_t font_spacing;
  float spacing;
  char* font_path;
}; //visible config for the figure


typedef struct Fconfig Fconfig;
typedef struct Figure Figure;
typedef struct Arena Arena;
typedef struct NC NC;

struct Figure {
  Arena* arena;
  char* title;
  Theme theme;
  bool force_update;
  Fconfig* fc;
  Axes* axes;
  Limit xlim;
  size_t axes_count;
};

double scale_to_limitf(double val, Limit src, Limit dest);
void scale_to_limit(const double *data, double *dest, size_t count, Limit srcl, Limit destl);
Limit get_lim(const double *data, size_t len);



//artist
Artist* artist_create(Axes *axes, ArtistType atype, float thick, RColor* colors, const char* label ,void *data);
void artist_line_set_data(Artist* artist,  double*data, size_t start, size_t len); //copy `data` of len `len` to &internal_buffer[`start`]
void artist_candle_set_data(Artist* artist, double*data, size_t start, size_t len);//copy `data` of len `len` to &internal_buffer[`start`]

//axes
void axes_show_legend(Axes* axes, LegendPosition lp);
void axes_set_yformatter(Axes* axes,char* format);

//figure
Figure* figure_create(size_t rows, size_t cols);
Figure *figure_createn(NC* nc); // manually set the nodes. it will call nc_compile(nc)
void figure_wait_for_InitWindow(Figure* figure);

void figure_set_theme(Figure* figure, Theme theme);

void figure_set_title(Figure* figure, char *title);
void figure_set_window_title(Figure* figure, char *wtitle);
void figure_set_view_data(Figure* figure, size_t len, char *xformat, double *xdata, size_t timeframe); //set view data
void figure_show(Figure* figure, char* window_title, int width, int height, int fps); //run the disp loop until the window is closed. for more control look at figure.h use the function below

void figure_update_xdata(Figure* figure, double* xdata, size_t timeframe, size_t start, size_t len); //update the xdata+start len len with new xdata of len len

void figure_show_cursors(Figure* figure);
void figure_show_legends(Figure* figure, LegendPosition lp);

//===control the event loop manually====
void raylib_init(Figure *figure,char *window_title,int width, int height, int fps); //calls initWindow
void raylib_init_loop(); //call every loop to set up keyboard events
void figure_update(Figure* figure); //call every loo[ to update the figure e.g axes etc


#endif //__RAYCANDLE_H__
