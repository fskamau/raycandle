#include "figure.h"
#include "raycandle.h"
#include "utils.h"
#include <string.h>
#include "node.h"
#include "axes.h"
#include "raylib.h"
#include <math.h>
#include <locale.h>

#define MAX_FIGURE_TITLE 256 // max figure title char
#define FIGURE_ARENA_SIZE 1024*1024*1024
#define NODES_ARENA_SIZE (size_t)1024*1024*8
#define DEFAULT_WINDOW_TITLE "rc"

#define BG_DEFAULT GetColor(0x161511ff)
#define FG_DEFAULT GetColor(0x8b8ba7ff)
#define VIEWPORT_INITIAL_LEN 120

#define DEFAULT_VELOCITY_DECAY 0.75f
#define MINIMUM_VELOCITY 0.005f //reset to zero if lower than this to avoid flickering
#define MAX_EMPTY_OFFSET 0.3f
#define ZOOMX_SCALE 10.f

#define THEME_DEFAULT (Theme){                  \
    .bg=BG_DEFAULT,                             \
      .fg=FG_DEFAULT,                           \
      .font_size=16,                            \
      .font_spacing=1,                          \
      .spacing=2,                               \
      }


#define AXES_YLABEL_MAX_LEN 128

static void  figure_compile(Figure* figure);
static void   figure_create_axes(Figure *figure);
static void load_font(Figure* figure);
static void update_xdata(Figure* figure, double* xdata, size_t timeframe, size_t start, size_t stop);
static void figure_update_xlim(Figure* figure);

static void insert_bar(bool top, Rect* src, Rect* dest, size_t height, float spacing);
static void  figure_draw_title(Figure *figure);
static void figure_mode_bar_draw(Figure *figure);
static void figure_handle_inputs(Figure* figure);
static void figure_draw_cursors(Figure *figure);
static void figure_axes_init(Figure* figure);

static Figure* figure_create_()
{
  setlocale(LC_ALL,"");
 Arena* arena=arena_create((size_t)FIGURE_ARENA_SIZE);
  Figure* figure=arena_alloc(arena,sizeof(Figure));
  memset(figure,0,sizeof(Figure));
  figure->theme=THEME_DEFAULT;
  figure->fc=arena_alloc(arena,sizeof(Fconfig));
  memset(figure->fc,0, sizeof(Fconfig));
  figure->fc->mode_bar.is_visible=1;
  figure->fc->signal_initialized=ready_signal_create(arena);
  figure->fc->vd.decay=DEFAULT_VELOCITY_DECAY;
  figure->fc->vd.max_offset=MAX_EMPTY_OFFSET;
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
  figure->axes_count=nc_compile(fc->nc);
  figure_compile(figure); //nodes & axes
  return figure;
}

Figure* figure_createn(NC* nc)
{
  ASSERT(nc);
  Figure* figure=figure_create_();
  figure->axes_count=nc_compile(nc);
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
  figure->fc->title_bar.is_visible=true;
  figure->force_update=true;
}


void raylib_init(Figure *figure,char *window_title,int width, int height, int fps)
{
  ASSERTM(figure->fc->state==FIGURE_STATE_CREATED, "show can only be called once\n");
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  SetTraceLogLevel(LOG_ERROR);
  InitWindow(width,height,window_title?window_title:DEFAULT_WINDOW_TITLE);
  SetTargetFPS(fps);
  load_font(figure);
  figure_axes_init(figure);
  figure->fc->state=FIGURE_STATE_RUNNING;
  ready_signal_set(figure->fc->signal_initialized);
}

void figure_wait_for_InitWindow(Figure* figure)
{
  ready_signal_wait(figure->fc->signal_initialized);
}


void raylib_init_loop(Figure* figure)
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
  if(IsWindowResized())figure->force_update=1;

}

void figure_update(Figure* figure){
  Fconfig *fc=figure->fc;
  NC* nc=figure->fc->nc;
  float spacing=figure->theme.spacing;
  Rect area=(Rect){0,0,GetScreenWidth(),GetScreenHeight()};
  if(fc->title_bar.is_visible)insert_bar(1, &area,&fc->title_bar.area, figure->theme.font_size,figure->theme.spacing);
  if(fc->mode_bar.is_visible)insert_bar(0, &area,&fc->mode_bar.area, figure->theme.font_size,figure->theme.spacing);
  figure_handle_inputs(figure);
  if(figure->force_update){
    nc_set_area(nc,figure->fc->rects,area,spacing);
    figure_update_xlim(figure);
  }
  for(size_t i=0;i<figure->axes_count;++i){
    if(figure->force_update)axes_update_ylim(figure->axes+i);
    axes_draw(figure->axes+i);
  }
  figure_draw_cursors(figure);
  figure_draw_title(figure);
  figure_mode_bar_draw(figure);
  figure->force_update=false;
}


void figure_show(Figure* figure, char* window_title, int width, int height, int fps)
{
  Fconfig* fc=figure->fc;
  ASSERTM(fc->state==FIGURE_STATE_CREATED,"figure_show called again\n");
  raylib_init(figure,window_title,width,height,fps);//figure state will now be FIGURE_STATE_RUUNING
  figure->force_update=true;//trigger updates the first loop
  while (!WindowShouldClose()) {
    raylib_init_loop(figure);
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

void figure_set_view_data(Figure* figure, size_t len, char *xformat, double *xdata, size_t timeframe)
{
  ASSERT(figure);
  ASSERT(len>0);
  ASSERT(xdata);
  ASSERT(xformat);
  ASSERT(timeframe!=0);
  Fconfig* fc=figure->fc;
  ASSERTM(!fc->vd.len,"view data can only be called once. use figure_update_xdata\n");
  fc->vd.len=len;
  fc->vd.vlen=MIN(VIEWPORT_INITIAL_LEN,len);
  fc->vd.stop=fc->vd.vlen;
  fc->timeframe=timeframe;
  fc->xdata=arena_alloc(figure->arena,sizeof(*fc->xdata)*len); //always alloc since update_xdata or update_xdatar maybe called elsewhere
  memcpy((fc->xdata=arena_alloc(figure->arena,sizeof(*fc->xdata)*len)),xdata,sizeof(*fc->xdata)*len);
  if(xformat){
    fc->xformat=str_createf_with_arena(figure->arena,"%s", xformat);
  }
}

void figure_update_xdata(Figure* figure, double* xdata, size_t timeframe)
{
  update_xdata(figure, xdata, timeframe,0,figure->fc->vd.len);
}


void figure_update_xdatar(Figure* figure, double* xdata,size_t timeframe, size_t start, size_t stop)
{
  update_xdata(figure, xdata,timeframe, start,stop);
}


void figure_show_cursors(Figure* figure)
{
  ASSERT(figure);
  figure->fc->show_cursors=1;
}


void figure_show_legends(Figure* figure, LegendPosition lp)
{
  ASSERT(figure);
  for(size_t i=0;i<figure->axes_count;++i)
    axes_show_legend(figure->axes+i,lp);
}


static void figure_compile(Figure* figure)
{
  ASSERT(figure->fc->nc);
  figure_create_axes(figure);
}


static void figure_create_axes(Figure *figure)
{
  Fconfig* fc=figure->fc;
  figure->axes=arena_alloc(figure->arena,sizeof(Axes)*figure->axes_count);
  fc->rects=arena_alloc(figure->arena,sizeof(Axes)*(figure->axes_count));
  memset( figure->axes,0,sizeof(Axes)*figure->axes_count);
  memset( fc->rects,0,sizeof(fc->rects)*figure->axes_count);
  for(size_t i=0;i<figure->axes_count;++i){
    figure->axes[i]=(Axes){
      .area=fc->rects+i,
      .fig=figure
    };
    figure->axes[i].ylabel.format=str_create_with_arena(figure->arena,AXES_YLABEL_MAX_LEN);
    str_append(figure->axes[i].ylabel.format,"%s","%f");
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

static void update_xdata(Figure* figure, double* xdata, size_t timeframe, size_t start, size_t stop)
{
  ASSERT(figure);
  ASSERT(xdata);
  ASSERT(timeframe>0);
  ASSERTM(figure->fc->vd.len,"call figure_set_view_data first\n");
  Fconfig* fc=figure->fc;
  ASSERTM(start<stop,"xdata index error\n");
  ASSERTM(stop<=fc->vd.len,"xdata index error\n");
  memcpy(fc->xdata+start,xdata,sizeof(*fc->xdata)*(stop-start));
  fc->timeframe=timeframe;
  
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
  Vd vd=fc->vd;
#define LEN 128
  char buf_[LEN]={0};
  char* buf=str_create(buf_,LEN);
  Theme theme=figure->theme;
  str_append(buf,"fps=%d ",GetFPS());
  Axes *aum=get_axes_under_mouse(figure);
  if(aum && aum->artist_len){
    int skip=0;
    float mx=(GetMouseX()-aum->area->x)/aum->area->w;
    float r=fabs(vd.offsetx)/vd.vlen;
    if(fabs(vd.offsetx)>1.0){
        skip=vd.offsetx<0.f?mx<r:mx>1.f-r;
        if(vd.offsetx<0.0)mx-=r;
        else mx+=r;
    }
    if(!skip){
      float index=mx*vd.vlen+vd.start;
      str_append(buf,"showing [%zu/%zu %zu] ",(size_t)index+1,vd.len, vd.vlen);
      size_t epoch= fc->xdata[(int)index]+(index-floor(index))*fc->timeframe;
      epoch2strftime(epoch,buf, fc->xformat);
    }
  }
  float x=align_text(&fc->font,buf,area.w, theme.font_size,theme.spacing, ALIGNMENT_LEFT);
  DrawTextEx(fc->font,buf,(Vector2){x+area.x,area.y+1},theme.font_size,theme.font_spacing,WHITE);
}

static void figure_handle_inputs(Figure* figure)
{
  Fconfig *fc=figure->fc;
  Vd vd=fc->vd;
  Vector2 *velocity=&vd.velocity;
  int need_update=0;
  Vector2 md=GetMouseDelta();
  Axes* aum=get_axes_under_mouse(figure);
  int has_artists=aum?aum->artist_len>0:0;
  int mbd=IsMouseButtonDown(MOUSE_BUTTON_LEFT);


  if(vd.aum!=aum){
    vd.aum=aum;
    vd.wait_mouse_up=true;
  }
  if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
    vd.wait_mouse_up=false;
    *velocity=(Vector2){0,0};
  }
  if(!vd.wait_mouse_up && aum && mbd){
    if(fabsf(md.x)>fabsf(md.y)) velocity->x+=(-md.x/aum->area->w);
    if(fabsf(md.y)>fabsf(md.x)) velocity->y=(-md.y/aum->area->h);
  }
  //dragging with mouse horiz
  if(velocity->x!=0.f){
    double max_offset=vd.vlen*vd.max_offset;
    vd.raw_start=fmin(vd.len+max_offset-vd.vlen,fmax(-max_offset,(vd.raw_start+velocity->x*vd.vlen)));
    size_t l=fmax(0.f, vd.raw_start)+vd.vlen;
    vd.offsetx=(vd.raw_start<0)?vd.raw_start:
      (l>vd.len)?vd.raw_start+vd.vlen-vd.len:
      vd.raw_start-floor(vd.raw_start);
    velocity->x*=vd.decay;
    if(fabsf(velocity->x)<MINIMUM_VELOCITY)velocity->x=0.f;
    need_update=1;
  }

  //dragging with mouse vert
  if(velocity->y != 0.f && md.y != 0.f &&has_artists &&mbd){;
    vd.offsety-=velocity->y;
    if(fabs(vd.offsety)>vd.max_offset)vd.offsety=vd.offsety>0?vd.max_offset:-vd.max_offset;
    need_update=1;
  }

  //reset with KEY_R
  if(IsKeyPressed(KEY_R)){
    velocity->x=velocity->y=0.f;
    vd.offsety=0.f;
    need_update=1;
  }

  //zoomx
  Vector2 wm=GetMouseWheelMoveV();
  if(aum && wm.y!=0.f && has_artists){
    int toc=(2.f*ZOOMX_SCALE*wm.y*-1);
    long long vlen=vd.vlen+toc;
    size_t pvlen=vd.vlen;
    vd.vlen=MIN(MAX(1,vlen),vd.len);
    vlen=vd.vlen-pvlen;
    float left=(GetMouseX()-aum->area->x)/aum->area->w*vlen;
    vd.raw_start-=left;
    /* vd.raw_start=fmin(vd.len-vd.vlen,fmax(0,(vd.raw_start+velocity->x*vd.vlen))); */

    double max_offset=vd.vlen*vd.max_offset;
    vd.raw_start=fmin(vd.len+max_offset,fmax(-max_offset,(vd.raw_start)));
    size_t l=fmax(0.f, vd.raw_start)+vd.vlen;
    vd.offsetx=(vd.raw_start<0)?vd.raw_start:
      (l>vd.len)?vd.raw_start+vd.vlen-vd.len:
      vd.raw_start-floor(vd.raw_start);

    need_update=1;
  }

  //finally
  {
    vd.start=fmax(0.f,vd.raw_start);
    size_t l=vd.start+vd.vlen;
    if(l>vd.len) vd.start = vd.len-vd.vlen;
    vd.stop=vd.start+vd.vlen;
    if(l<vd.len)vd.stop+=1;
  }
  if(need_update)figure->force_update=need_update;
  fc->vd=vd;
}


static void  figure_draw_title(Figure *figure)
{
  Fconfig *fc=figure->fc;
  if(!fc->title_bar.is_visible || !figure->title)return;
  Theme theme=figure->theme;
  Rect area=fc->title_bar.area;
  float x=align_text(&fc->font,figure->title,area.w, theme.font_size,theme.spacing, ALIGNMENT_CENTER);
  DrawTextEx(fc->font,figure->title,(Vector2){x+area.x,area.y+1},theme.font_size,theme.font_spacing,Fade(theme.fg,1.0));  
}


static void  figure_draw_cursors(Figure *figure)
{
  if(!figure->fc->show_cursors)return;
  Theme theme=figure->theme;
  Vector2 mp=GetMousePosition();
  Axes* aum=get_axes_under_mouse(figure);
  if(!aum||!aum->artist_len)return;
  Rect area=*aum->area;
  double mreal=scale_to_limitf(mp.y, (Limit){area.y,area.y+area.h},aum->lim);
  float px=scale_to_limitf(mp.x, (Limit){area.x,area.x+area.w},(Limit){0.f,1.f});
  Color c=Fade(theme.fg,0.5);
  for(size_t i=0;i<figure->axes_count;++i){
    Axes axes=figure->axes[i];
    if(!axes.artist_len)continue;
    Rect areax=*axes.area;
    float pixely=scale_to_limitf(mreal,axes.lim,(Limit){areax.y,areax.y+areax.h});
    int pxv=px*areax.w+areax.x;
    DrawLine(pxv,areax.y,pxv,areax.y+areax.h,c);
    DrawLine(areax.x,pixely,areax.x+areax.w,pixely,c);
  }
}


static void figure_axes_init(Figure* figure)
{
  const int buflen=256;  
  char buf_[buflen];
  Theme theme=figure->theme;
  Font font=figure->fc->font;
  char* buf=str_create(buf_,buflen);
  for(size_t i=0;i<figure->axes_count;++i){
    Axes* axes=figure->axes+i;
    if(!axes->artist_len)continue;
    str_clear(buf);
    for(size_t x=0;x<axes->legend.llen;++x)str_append(buf,"%s"," ");
    axes->legend.llen=MeasureTextEx(font, buf, theme.font_size,theme.font_spacing).x;
  }    
}
