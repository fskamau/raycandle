#include "utils.h"
#include "axes.h"
#include "ready_signal.h"
#include "raylib.h"
#include "node.h"
#include "raycandle.h"

typedef enum{
  FIGURE_STATE_CREATED=0,
  FIGURE_STATE_RUNNING,
  FIGURE_STATE_STOPPED,  
}FigureState;


typedef  struct Figure Figure;
typedef struct NC NC;
typedef struct Vd Vd; //view data


struct Vd{
  size_t len;
  double rstart;
  size_t start;
  size_t vlen,draw_len;
  double rvlen;
  float offsetx, offsety;
  float panx_velocity;
  float zoomx_velocity;
  Vector2 zoom;
  Axes* aum;
  int wait_mouse_up;
  int redraw_modebar;

};
  


typedef struct{
  Rect area;
  int is_visible;
}Bar;

struct Fconfig {
  NC *nc;
  char *font_path;
  ReadySignal* signal_initialized;
  FigureState state;
  Rect* rects;
  Vd vd;
  double* xdata;
  char* xformat;
  Bar title_bar;
  Bar mode_bar;
  int show_cursors;
  size_t timeframe;
};
 
