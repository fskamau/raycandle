/*
A simple library for drawing candlesticks with an api somewhat similar to matplotlib
*/

#ifndef __RAYCANDLE__
#define __RAYCANDLE__
#define RAYCANDLE_DEBUG 0

#include "raylib.h"
#include <stddef.h>
#include <stdint.h>

#define RANDOM_PIXEL (int)(rand()%(255+1))
#define RANDOM_COLOR() ((Color){RANDOM_PIXEL, RANDOM_PIXEL, RANDOM_PIXEL, 255})
#define RC_LEGEND_ICON_WIDTH 50
#define RC_LEGEND_PADDING 30
#define AXES_FRAME_THICK 1


typedef struct {uint8_t a,b,c,d;}CFFI_Color;
#define CFFI_Color Color
typedef struct{float a,b;}CFFI_Vector2;
#define CFFI_Vector2 Vector2
typedef char* CFFI_Str ;


typedef enum {
  SCREEN_DIMENSION_STATE_DEFAULT = 'd',   // only exists before InitWindow is called
  SCREEN_DIMENSION_STATE_CHANGED = 'c',   // when you resize the window
  SCREEN_DIMENSION_STATE_UNCHANGED = 'u', // if previous window dimension has not changed
} ScreenDimensionState;

typedef enum {  
  ARTIST_TYPE_LINE = 0,
  ARTIST_TYPE_CANDLE = 1,
} ArtistType;

typedef enum{
  LINE_TYPE_S_LINE, //segmented line
  LINE_TYPE_H_LINE, //Horizontal line that extend whole axes
  LINE_TYPE_V_LINE, //vertical line that extend whole axes
}LINE_TYPE;

typedef enum {
  FORMATTER_LINEAR_FORMATTER = 0,
  FORMATTER_TIME_FORMATTER = 1,
  FORMATTER_NULL_FORMATTER = 2,
} FormatterType;

typedef struct Artist Artist;
typedef struct Axes Axes;
typedef struct Figure Figure;
typedef struct Limit Limit;

typedef struct {
  size_t cols;
  double *ydata; //array of len mostly figure->dragger->len_data*cols.
  char *label;   // col labels, of len cols
} Gdata;

struct Limit {
  double limit_min, limit_max, diff;
  bool is_static;
};


typedef struct {
  char *format;
  size_t flen;
  Limit limit;
  FormatterType ftype;
} Locator;

typedef struct {
  size_t _len;//len of data to being plotted
  size_t start;//start of currently visible data
  size_t vlen;//len of visible data on the screen >=1
  size_t ulen;//len of data points to move when updating
  size_t timeframe;//current timeframe in seconds
  Locator locator;
  double * xdata;//xdata, in epochs shared by all axes
  double * xdata_shared;//scaled xdata , in epochs shared by all axes  
}Dragger;

void set_dragger(Figure* figure,size_t len,size_t timeframe,double* xdata,FormatterType ftype,char* format);

struct Artist {
  Axes *parent;
  Artist *next; // next artist if any
  void *data;                // any special data that maybe needed by the artist
  Gdata gdata;
  float thickness;
  ArtistType artist_type;
  CFFI_Color *color;
  bool ylim_consider;//whether this artist will be used to find ylims
};


typedef struct{
  double *data;
  LINE_TYPE line_type;
}LineData;


typedef enum {
  LEGEND_POSITION_NO_LEGEND = 0,
  LEGEND_POSITION_TOP_LEFT = 1,
  LEGEND_POSITION_TOP_RIGHT = 2,
  LEGEND_POSITION_BOTTOM_LEFT = 3,
  LEGEND_POSITION_BOTTOM_RIGHT = 4,
} LegendPosition;

typedef struct {
  LegendPosition legend_position;
  int width, height;
} Legend;

struct Axes {
  size_t startX, startY, width, height;
  double *xdata_buffer; // x-axis data
  float ylabel_len,ylabel_padding;
  Figure *parent;
  char *title;
  Artist *artist;
  size_t artist_len;
  float padding;
  Locator ylocator; // transforms pixel positions to&from data values
  Legend legend;
  CFFI_Color facecolor;
  char label;
  uint8_t tableau_t10_index;
};


typedef struct {
  Axes *axes; //current Axes where the cursor is
  float accumulator; // diffx accumulated here until they exceed x-spacing
  int diffx;// difference between mouse-x movements
  bool down, wait_up;
} MouseInfo;



struct Figure {
  int width;
  int height;
  int fps;
  int font_size;
  CFFI_Vector2 label_font_ex;
  int font_spacing;
  float zoomx_padding;
  float border_percentage;
  Dragger dragger;
  char *title,*window_title;
  size_t axes_len;
  size_t rows;
  size_t cols;
  size_t *axes_skels;
  size_t *axes_skels_copy;
  size_t label_length;
  Axes *axes;
  size_t *border_dimensions;
  void* font,*label_font;
  CFFI_Str font_path;
  MouseInfo mouseinfo;
  CFFI_Color axes_frame_color;
  CFFI_Color background_color;
  CFFI_Color text_color;
  ScreenDimensionState sds;
  bool show_cursors,force_update,has_dragger,clear_screen,show_xlabels,show_ylabels;
};



/**
   args
   ----
   fig_size: (int[]){screenwidth,screenheight}
   title: pointer to a string or null for creating a title
   axes_len: length of axes
   rows: axes skeleton horizontal downwards span
   cols: axes skeleton vertical rightwards span
   visible_data: data viewable on the screen. if 0  no data will to be drawn
   update_len: data to update if an update is triggered by pressing specific keys. If 0 no data will be updated
   axes_skel: pointer to axes skeleton of length axes_skel*4 (startx,starty,spanx,spany)
   border_percentage: screen dimension percentages to be used as border *must be a percentage* e.g 0.1
   fps: integer describing the fps to apply.
        if 0 no fps will be applied else
        fps will be set to fps if fps is non-negative
  font_size: font size
  font-spacing: font spacing
  show_cursors: whether to draw cursors. cursor are straight lines following the mouse pointer
  clear_scrren: if true, figure will  be filled with background only. bound to KEY_I
*/
Figure *create_figure(char* figskel,int *fig_size, char *window_title, CFFI_Color background_color,float border_percentage, int fps, size_t font_size, int font_spacing,char* font_path);
/**
show calls InitWindow which will then draw the figure on screen
 */
void show(Figure *figure); 
void figure_set_title(Figure *figure, char *title);
void axes_set_title(Axes *axes, char *title); // set title of the axes
void axes_set_yformatter(Axes *axes, char *formatter);
void axes_show_legend(Axes *axes, LegendPosition legend_position);
void figure_set_xlim(Figure *figure, double xminmax[2]);
void update_timeframe(Figure* figure,size_t timeframe); //set a new timeframe
void axes_set_ylim(Axes *axes, double yminmax[2]);
Artist *create_artist(Axes *axes, ArtistType artist_type, Gdata gdata, double ydata_minmax[2], float thickness, CFFI_Color *color,void*config);
void update_from_position(size_t new_position, Figure *figure); // sets the current postion to `new_position` and updates all artists
Axes* get_axes_under_mouse(Figure* figure);
void cm_free_all_();// frees all allocated memory



#define RC_LABEL_FONT_SIZE 14
#define RC_MAX_PLOTTABLE_LEN 500
#define RC_UPDATE_LEN 10
#define RC_INITIAL_VISIBLE_DATA 120

#endif // __RAYCANDLE__
