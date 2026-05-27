#ifndef __RAYCANDLE_H__
#define __RAYCANDLE_H__

/* #include "axes.h" */
/* #include "artist.h" */
#include "node.h"
#include "raylib.h"

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

typedef struct{
  Rect area;
  char *format;
}Label;

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
struct Figure {
  Arena* arena;
  char* title;
  Theme theme;
  bool force_update;
  Fconfig* fc;
  Axes* axes;
  Limit xlim;
};

double scale_to_limitf(double val, Limit src, Limit dest);
void scale_to_limit(const double *data, double *dest, size_t count, Limit srcl, Limit destl);
Limit get_lim(const double *data, size_t len);



//artist
Artist* artist_create(Axes *axes, ArtistType atype, float thick, Color* colors, const char* label ,void *data);

//axes
void axes_show_legend(Axes* axes, LegendPosition lp);

//figure
Figure* figure_create(size_t rows, size_t cols);
Figure *figure_createn(NC* nc); // manually set the nodes. it will call nc_compile(nc)

void figure_set_theme(Figure* figure, Theme theme);

void figure_set_title(Figure* figure, char *title);
void figure_set_window_title(Figure* figure, char *wtitle);
void figure_set_view_data(Figure* figure, size_t len, char *xformat, double *xdata, size_t timeframe); //set view data
void figure_show(Figure* figure, char* window_title, int width, int height, int fps); //run the disp loop until the window is closed. for more control look at figure.h use the function below

void figure_update_xdata(Figure* figure, double* xdata);//update the xdata with new data
void figure_update_xdatar(Figure* figure, double* xdata,size_t start, size_t stop);//update the xdata with new data with a range

void figure_show_cursors(Figure* figure);
void figure_show_legends(Figure* figure, LegendPosition lp);

//===control the event loop manually====
void raylib_init(Figure *figure,char *window_title,int width, int height, int fps); //calls initWindow
void raylib_init_loop(); //call every loop to set up keyboard events
void figure_update(Figure* figure); //call every loo[ to update the figure e.g axes etc


#endif //__RAYCANDLE_H__
