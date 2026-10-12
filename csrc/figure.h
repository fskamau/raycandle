#include "utils.h"
#include "axes.h"
#include "ready_signal.h"
#include "raylib.h"
#include "node.h"
#include "raycandle.h"
#include "view.h"


typedef enum{
  FIGURE_STATE_CREATED=0,
  FIGURE_STATE_RUNNING,
  FIGURE_STATE_STOPPED,  
}FigureState;


typedef  struct Figure Figure;
typedef struct NC NC;
typedef struct Vd Vd; //view data

typedef struct JID JID;
struct JID {
  double rindex;
  char *epoch;
};//jump index data
typedef struct{
  size_t len;
  size_t cap;
  JID* jid;
  Rect area;
}JIS;//jump index store;


struct Vd{
  size_t len;
  double rstart;
  size_t start;
  size_t vlen,draw_len;
  size_t rvlen;
  float offsetx, offsety;
  float panx_velocity;
  float zoomx_velocity;
  Vector2 zoom;
  Axes* aum;
  int wait_mouse_up;
  int redraw_modebar;
  JIS* jis;

};
  
typedef struct{
  Rect area;
  int is_visible;
}Bar;


struct AxesView{
  Artist *artist;
  size_t artist_len;
  Legend legend;
  Limit lim;
  Label ylabel;
  size_t ylabel_len, ylabel_padding;
  size_t tableau_t10_index;
  int lim_fixed;
};


typedef struct FigureView FigureView;
struct FigureView{
  bool visible;
  BarSeries *bs;
  AxesView *av;
  /* char* title; */
  Vd vd;
  FigureView *next;
};



struct Fconfig {
  NC *nc;
  ReadySignal* signal_initialized;  
  FigureState state;
  size_t fv_count;
  Rect* rects;
  char* xformat;
  FigureView *fv;//base view
  Bar title_bar;
  Bar mode_bar;
  int show_cursors;
  bool figure_view_locked;
};
 
