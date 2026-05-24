#include "figure.h"
#include "raycandle.h"
#include "utils.h"
#include <string.h>
#include "node.h"
#include "axes.h"
#include "raylib.h"
#include <math.h>

#define MAX_FIGURE_TITLE 256 // max figure title char
#define FIGURE_ARENA_SIZE 1024*1024
#define NODES_ARENA_SIZE 1024*1024*8
#define DEFAULT_WINDOW_TITLE "rc"

#define BG_DEFAULT GetColor(0x060511ff)
#define FG_DEFAULT GetColor(0x8b8ba7ff)
#define VIEWPORT_INITIAL_LEN 100

#define DEFAULT_VELOCITY_DECAY 0.7
#define MINIMUM_VELOCITY 0.02 //reset to zero if lower than this
#define VELOCITY_SCALE 1.9

#define THEME_DEFAULT (Theme){                  \
    .bg=BG_DEFAULT,                             \
      .fg=FG_DEFAULT,                           \
      .font_size=16,                            \
      .font_spacing=1,                          \
      .spacing=2,                               \
      }

static void  figure_compile(Figure* figure);
static void   figure_create_axes(Figure *figure);
static void load_font(Figure* figure);
static inline void validate_figure_update_xdata_args(Figure* figure, double* xdata);
static void figure_update_xlim(Figure* figure);

static void insert_bar(bool top, Rect* src, Rect* dest, size_t height, float spacing);
static void figure_mode_bar_draw(Figure *figure);
static void figure_handle_inputs(Figure* figure);
static Axes* get_axes_under_mouse(Figure* figure);

static Figure* figure_create_()
{
 Arena* arena=arena_create((size_t)FIGURE_ARENA_SIZE);
  Figure* figure=arena_alloc(arena,sizeof(Figure));
  memset(figure,0,sizeof(Figure));
  figure->theme=THEME_DEFAULT;
  figure->fc=arena_alloc(arena,sizeof(Fconfig));
  memset(figure->fc,0, sizeof(Fconfig));
  figure->fc->mode_bar.is_visible=1;
  figure->fc->signal_initialized=ready_signal_create(arena);
  figure->fc->vd.decay=DEFAULT_VELOCITY_DECAY;  
  figure->arena=arena;
  return figure;
}

Figure* figure_create(size_t rows, size_t cols)
{
  Figure* figure=figure_create_();
  Fconfig* fc=figure->fc;
  fc->nc=nc_create(NODES_ARENA_SIZE);
  if(rows!=1&&cols!=1)//to allow figure_show to trigger
    nc_grid(fc->nc, fc->nc->root, rows,cols);
  figure->fc->axes_count=nc_compile(fc->nc);
  figure_compile(figure); //nodes & axes
  
  return figure;
}

Figure* figure_createn(NC* nc)
{
  ASSERT(nc);
  Figure* figure=figure_create_();
  figure->fc->axes_count=nc_compile(nc);
  figure->fc->nc=nc;
  figure_compile(figure);
  return figure;
}


void figure_set_title(Figure* figure, char *title)
{
  ASSERTM(title&&figure,"null argument figure or title\n");
  if(!figure->title)figure->title=str_create_with_arena(figure->arena, MAX_FIGURE_TITLE);
  str_clear(figure->title);
  str_append(figure->title, "%s",title);
  figure->force_update=true;
}


void raylib_init(Figure *figure,char *window_title,int width, int height, int fps)
{
  ASSERTM(figure->fc->state==FIGURE_STATE_CREATED, "show can only be called once\n");
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  SetTraceLogLevel(LOG_ERROR);
  InitWindow( height,width, window_title?window_title:DEFAULT_WINDOW_TITLE);
  SetTargetFPS(fps);
  load_font(figure);
  figure->fc->state=FIGURE_STATE_RUNNING;
  ready_signal_set(figure->fc->signal_initialized);
}


void raylib_init_loop()
{
  // keyboard
  //  F -> Maximize
  if (IsKeyPressed(KEY_F)) {
    if (IsWindowMaximized()) {
      RestoreWindow();
    } else {
      MaximizeWindow();
    }
  }
  // Q -> exit
  SetExitKey(KEY_Q);
}

void figure_update(Figure* figure){
  Fconfig *fc=figure->fc;
  NC* nc=figure->fc->nc;
  float spacing=figure->theme.spacing;    
  Rect area=(Rect){0,0,GetScreenWidth(),GetScreenHeight()};
  if(fc->title_bar.is_visible)insert_bar(1, &area,&fc->mode_bar.area, figure->theme.font_size,figure->theme.spacing);
  if(fc->mode_bar.is_visible)insert_bar(0, &area,&fc->mode_bar.area, figure->theme.font_size,figure->theme.spacing);     
  nc_set_area(nc,figure->fc->rects,area,spacing);
  if(figure->force_update)figure_update_xlim(figure);    
  for(size_t i=0;i<fc->axes_count;++i){
    axes_draw(figure->axes+i);
  }
  figure_mode_bar_draw(figure);
    figure_handle_inputs(figure);   

  /* figure->force_update=false;     */
}


void figure_show(Figure* figure, char* window_title, int width, int height, int fps)
{
  Fconfig* fc=figure->fc;
  ASSERTM(fc->state==FIGURE_STATE_CREATED,"figure_show called again\n");
  raylib_init(figure,window_title,width,height,fps);//figure state will now be FIGURE_STATE_RUUNING
  figure->force_update=true;//trigger updates the first loop    
  while (!WindowShouldClose()) {
    raylib_init_loop();
    BeginDrawing();
    ClearBackground(figure->theme.bg);
    figure_update(figure);    
    EndDrawing();
  }
  CloseWindow();
}

void figure_set_theme(Figure* figure, Theme theme)
{
  ASSERT(figure);
  if(theme.font_path){
    ASSERTM(!figure->fc->font_path,"figure_set_theme can only load 1 font; set font_path to NULL\n");
    ASSERTM(figure->fc->state==FIGURE_STATE_CREATED,"can only set font when before figure_show\n");
  }
  figure->fc->font_path=str_createf("%s",theme.font_path);
  theme.font_path=NULL;
  figure->theme=theme;
  figure->force_update=true;
}

void figure_set_view_data(Figure* figure, size_t len, char *xformat, double *xdata)
{
  ASSERT(figure);
  ASSERT(len>0);
  ASSERT(xdata);
  Fconfig* fc=figure->fc;    
  ASSERTM(!fc->vd.len,"view data can only be called once. use figure_set_xdata\n");
  fc->vd.len=len;
  fc->vd.vlen=MIN(VIEWPORT_INITIAL_LEN,len);
  fc->vd.stop=fc->vd.vlen;
  fc->xdata=arena_alloc(figure->arena,sizeof(*fc->xdata)*len); //always alloc since update_xdata or update_xdatar maybe called elsewhere
  memcpy((fc->xdata=arena_alloc(figure->arena,sizeof(*fc->xdata)*len)),xdata,sizeof(*fc->xdata)*len);
  if(xformat){
    fc->xformat=str_createf("%s", xformat);
  }
}


void figure_update_xdatar(Figure* figure, double* xdata,size_t start, size_t stop)
{
  validate_figure_update_xdata_args(figure, xdata);
  Fconfig* fc=figure->fc;
  ASSERTM(start<stop,"index error\n");
  ASSERTM(stop<=fc->vd.len,"index error\n");
  memcpy(fc->xdata+start,xdata,sizeof(*fc->xdata)*(stop-start));
}

void figure_update_xdata(Figure* figure, double* xdata)
{
  validate_figure_update_xdata_args(figure, xdata);
  figure_update_xdatar(figure,xdata,0,figure->fc->vd.len);
}

static void figure_compile(Figure* figure)
{
  ASSERT(figure->fc->nc);
  figure_create_axes(figure);    
}


static void figure_create_axes(Figure *figure)
{
  Fconfig* fc=figure->fc;
  figure->axes=arena_alloc(figure->arena,sizeof(Axes)*fc->axes_count);
  fc->rects=arena_alloc(figure->arena,sizeof(Axes)*(fc->axes_count));
  memset( figure->axes,0,sizeof(Axes)*fc->axes_count);
  memset( fc->rects,0,sizeof(fc->rects)*fc->axes_count);
  for(size_t i=0;i<fc->axes_count;++i){    
    figure->axes[i]=(Axes){
      .area=fc->rects+i,
      .fig=figure
    };           
  }
}

static void load_font(Figure* figure)
{
  Fconfig* fc=figure->fc;
  Font* font=&figure->fc->font,temp;
  *font=GetFontDefault();
  if(fc->font_path){
    temp=LoadFont(fc->font_path);
    if(temp.baseSize!=0){
      *font=temp;
    }
    str_free(fc->font_path);
  }
}

static inline void validate_figure_update_xdata_args(Figure* figure, double* xdata)
{
  ASSERT(figure);
  ASSERT(xdata);
  ASSERTM(figure->fc->vd.len,"call figure_set_view_data first\n");
}

static void figure_update_xlim(Figure* figure)
{
  Fconfig *fc=figure->fc;
  figure->xlim=get_lim(fc->xdata+fc->vd.start,fc->vd.len);
}


static void insert_bar(bool top, Rect* src, Rect* dest, size_t height, float spacing)
{
  ASSERT(src&&dest);
    const float lw = 1.0f;
    dest->x = src->x + spacing + lw;
    dest->w = src->w - (spacing * 2.0f) - (lw * 2.0f);
    dest->h = (float)height;
    if (top) {
        dest->y = src->y + lw;
        float total_consumed = (float)height + spacing + lw;
        src->y += total_consumed;
        src->h -= total_consumed;
    } else {
        dest->y = (src->y + src->h) - (float)height - lw;
        float total_consumed = (float)height + spacing + lw;
        src->h -= total_consumed;
    }
}


static void figure_mode_bar_draw(Figure *figure)
{
  Fconfig *fc=figure->fc;
  if(!fc->mode_bar.is_visible)return;
  Rect area=fc->mode_bar.area;
  DrawRectangleLines(NODE_RECT_UNWRAP(area),figure->theme.fg);
#define LEN 128
  char buf_[LEN]={0};
  char* buf=str_create(buf_,LEN);
  Theme theme=figure->theme;
  Vector2 v=GetMouseDelta();
  str_append(buf," X:%d Y:%d Delta(%f %f)", GetMouseX(),GetMouseY(),v.x,v.y);
  DrawTextEx(fc->font,buf,(Vector2){area.x+1,area.y+1},theme.font_size,theme.font_spacing,theme.fg);
}

static Axes* get_axes_under_mouse(Figure* figure)
{
  for(size_t i=0;i<figure->fc->axes_count;++i){
    if(CheckCollisionPointRec(GetMousePosition(),NODE_RECT2RECTANGLE(figure->fc->rects[i])))
      return figure->axes+i;
  }
  return NULL;
}
  
static void figure_handle_inputs(Figure* figure)
{
  Fconfig *fc=figure->fc;
  Vd vd=fc->vd;
  Vector2 *velocity=&vd.velocity;
  int need_update=0;
  if(IsWindowResized())need_update=1;
  Vector2 md=GetMouseDelta();
  Axes* aum=get_axes_under_mouse(figure);
  int xmdmax=fabsf(md.x)>fabsf(md.y);
  
  //dragging with mouse horiz && vertical
  if(vd.aum!=aum){
    vd.aum=aum;
    vd.wait_mouse_up=true;
  }
  if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))vd.wait_mouse_up=false;
  if(!vd.wait_mouse_up && aum && IsMouseButtonDown(MOUSE_BUTTON_LEFT)){
    if(xmdmax){
      velocity->x+=(-md.x/aum->area->w)*VELOCITY_SCALE;
    }if(fabsf(md.y)>fabsf(md.x)){
      velocity->y+=(-md.y/aum->area->h)*VELOCITY_SCALE;
    }
  }
  if(fabsf(velocity->x)>MINIMUM_VELOCITY)velocity->x*=vd.decay;
  if(fabsf(velocity->y)>MINIMUM_VELOCITY)velocity->y*=vd.decay;
  double myd;
  if(fabsf(velocity->x)>MINIMUM_VELOCITY){
    myd=fmax(0.f,vd.start+velocity->x*vd.vlen);
    if(myd+vd.vlen>vd.len){
        myd=(double)vd.len-vd.vlen;
        velocity->x=0.f;
      }
    vd.stop=myd+vd.vlen;
    vd.start=myd;
    if(vd.start==0)velocity->x=0.f;
    vd.xoffset=velocity->x*vd.vlen;
    vd.xoffset=vd.xoffset-floor(vd.xoffset);
    need_update=1;
  }
    
    
 
  if(!figure->force_update)figure->force_update=need_update;
  fc->vd=vd;
}
