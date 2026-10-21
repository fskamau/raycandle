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
typedef struct ReadySignal ReadySignal;
typedef struct {
  size_t llen;
  LegendPosition lp;
}Legend;


#ifndef RAYLIB_H

// Color port from raylib to cffi
typedef struct {
  unsigned char r,g,b,a;
}RColor;
#define RColor Color

typedef RColor Color;
typedef void Font;
typedef struct Arena Arena;
Arena *arena_create(size_t bytes);

#endif // RAYLIB_H


typedef struct {
  double min,max;
}Limit;


typedef struct FigureView FigureView;
typedef struct Artist Artist;
typedef struct ArtistVTable ArtistVTable;

typedef enum   {
  ARTIST_TYPE_LINE ,
  ARTIST_TYPE_CANDLE ,
  ARTIST_TYPE_STUDY ,
}ArtistType;

/*
  if is_splined then data len is that set by figure_set_view_data
  else:data len is double[4]  with {startx,endx, starty, endy}
 */
typedef struct {
  int is_splined;
  float* data;
}LineData;

typedef struct Axes Axes;
struct Artist{
  ArtistType atype;
  void *data;
  Artist* next;
  Color* colors;
  Axes* axes;
  ArtistVTable* avt;
  FigureView* view; /* immutable owner, not the currently selected view */
  size_t data_len; /* float count, not byte count */
  char *label;
  float thick;
  bool is_limitable;
};

typedef struct{
  Rect area;
  char *format;
}Label;

typedef struct AxesView AxesView;
struct Axes{
  Rect* area;
  AxesView *av;
  float w;
  Figure *fig;

};

typedef struct Theme Theme;
struct Theme{
  Color bg,fg;
  size_t font_size;
  size_t font_spacing;
  float spacing;
  char* font_path, *icon_path;
  bool show_ylabel;
}; //visible config for the figure

typedef struct FigureView FigureView;
typedef struct Fconfig Fconfig;
typedef struct Figure Figure;
typedef struct Arena Arena;
typedef struct NC NC;


typedef struct{
  bool on;
  size_t axes_index;
  int x[10];
}MC;

struct Figure {
  Arena* arena;
  ReadySignal* data_mutex;
  Theme theme;
  Fconfig* fc;
  Axes* axes;
  FigureView *fv;
  size_t axes_count;
  Font* font;
  bool owns_font;
  Limit xlim;
  int redraw_count;
  int redraw_modebar;
  MC mc;
};

double scale_to_limitf(double val, Limit src, Limit dest);
void scale_to_limit(const double *data, double *dest, size_t count, Limit srcl, Limit destl);
Limit get_lim(const double *data, size_t len);
Limit get_limf(const float *data, size_t len);



//artist
Artist* artist_create(Axes *axes, ArtistType atype, float thick, Color* colors, const char* label ,void *data);
void artist_line_set_data(Artist* artist,  float* data, size_t start, size_t len); //copy `data` of len `len` to &internal_buffer[`start`]
void artist_candle_set_data(Artist* artist, float*data, size_t start, size_t len);//copy `data` of len `len` to &internal_buffer[`start`]

/* C owns the storage until figure_destroy(). Never resize the returned memory. */
float* artist_get_data_ptr(Artist* artist);
size_t artist_get_data_len(Artist* artist);
void artist_line_shift(Artist* artist, size_t count);
void artist_invalidate(Artist* artist);
void artist_set_limitable(Artist* artist, int enabled);
void artist_set_alpha(Artist* artist, unsigned char alpha);
int artist_candle_update_tick(Artist* artist, double epoch, float price);

//axes
void axes_show_legend(Axes* axes, LegendPosition lp);
void axes_set_yformatter(Axes* axes,char* format);
void axes_set_ylim(Axes* axes,double min, double max);

//figure
Figure* figure_create(size_t rows, size_t cols);
Figure *figure_createn(NC* nc); // manually set the nodes. it will call nc_compile(nc)
void figure_wait_for_InitWindow(Figure* figure);
void figure_destroy(Figure* figure);
Axes* figure_get_axes(Figure* figure, size_t index);
size_t figure_get_view_count(Figure* figure);
size_t figure_get_active_view(Figure* figure);
size_t figure_get_view_len(Figure* figure, size_t view_id);
const double* figure_view_get_epochs(Figure* figure, size_t view_id);
void figure_invalidate(Figure* figure);
void figure_lock(Figure* figure);
void figure_unlock(Figure* figure);
void figure_set_font_path(Figure* figure, const char* path);
/* initialize a selected view once; epochs are copied and kept independent */
void figure_view_set_data(Figure* figure, size_t view_id, size_t len, const double* epochs, size_t timeframe, const char* title);
void figure_view_update_xdata(Figure* figure, size_t view_id, const double* epochs, size_t start, size_t count);


void figure_set_theme(Figure* figure, Theme theme);
void figure_set_xformat(Figure* figure, const char* xformat); //change default timeseries xformat
void figure_set_window_title(Figure* figure, char *wtitle);
void figure_show(Figure* figure, char* window_title, int width, int height, int fps); //run the disp loop until the window is closed. for more control look at figure.h use the function below



void figure_show_cursors(Figure* figure);
void figure_show_legends(Figure* figure, LegendPosition lp);

//===control the event loop manually====
void raylib_init(Figure *figure,char *window_title,int width, int height, int fps); //calls initWindow
void raylib_init_loop(Figure* figure); //call every loop to set up keyboard events
void figure_update(Figure* figure); //call every loo[ to update the figure e.g axes etc


typedef struct BarSeries BarSeries;
typedef struct {
  float *o,*h,*l,*c;
} Candles;


size_t figure_add_view(Figure* figure); // creates an independent empty view and selects it
void figure_move_view_on_top(Figure* figure, size_t view_id);
void figure_print_views(Figure* figure);

#endif //__RAYCANDLE_H__
