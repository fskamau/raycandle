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
  size_t start;
  size_t stop; //exclusive
  size_t vlen;
  size_t eff_vlen;
  size_t ulen;
  size_t zoomx;  
  Vector2 velocity;
  Axes* aum;
  int wait_mouse_up;
  float decay;
  double raw_start;
  float offsetx,offsety;
  float max_offset;
  float scaley;
};//view data


typedef struct{
  Rect area;
  int is_visible;
}Bar;

struct Fconfig {
  NC *nc;
  char *font_path;
  ReadySignal* signal_initialized;
  FigureState state;
  bool update;
  Rect* rects;
  Vd vd;
  double* xdata;
  char* xformat;
  Bar title_bar;
  Bar mode_bar;
  int show_cursors;
  size_t timeframe;
};
 
