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
  size_t ulen;
  Vector2 velocity;
  Axes* aum;
  int wait_mouse_up;
  float decay;
  double raw_start;
  float offsetx,offsety;
  float max_offset;
};//view data


typedef struct{
  Rect area;
  int is_visible;
}Bar;

struct Fconfig {
  NC *nc;
  char *font_path;
  ReadySignal* signal_initialized;
  Font font;
  FigureState state;
  bool update;
  size_t axes_count;
  Rect* rects;
  Vd vd;
  double* xdata;
  char* xformat;
  Bar title_bar;
  Bar mode_bar;
};


  
Figure* figure_create(size_t rows, size_t cols);
Figure *figure_createn(NC* nc); // manually set the nodes. it will call nc_compile(nc)
void figure_set_title(Figure* figure, char *title);
void figure_set_window_title(Figure* figure, char *wtitle);
void figure_show(Figure* figure, char* window_title, int width, int height, int fps); //run the disp loop until the window is closed. for finegrained control use the function below

//===control the event loop manually====
void raylib_init(Figure *figure,char *window_title,int width, int height, int fps); //calls initWindow
void raylib_init_loop(); //call every loop to set up keyboard events
void figure_update(Figure* figure); //call every loo[ to update the figure e.g axes etc




void figure_update_xdata(Figure* figure, double* xdata);//update the xdata with new data
void figure_update_xdatar(Figure* figure, double* xdata,size_t start, size_t stop);//update the xdata with new data with a range
