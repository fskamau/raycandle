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
#define FIGURE_ARENA_SIZE 1000*1024*1024
#define NODES_ARENA_SIZE (size_t)1024*8
#define DEFAULT_WINDOW_TITLE "rc"

#define BG_DEFAULT GetColor(0x161511ff)
#define FG_DEFAULT GetColor(0x8b8ba7ff)
#define VIEWPORT_INITIAL_LEN 120


#define MINIMUM_VLEN 2 //could be 1 but limit would be 0

#define MAX_ZOOMY 90.0
#define ZOOMY_SCALE 5.0

#define ARROW_KEYS_SCALE 0.2

#define THEME_DEFAULT (Theme){                  \
    .bg=BG_DEFAULT,                             \
      .fg=FG_DEFAULT,                           \
      .font_size=16,                            \
      .font_spacing=1,                          \
      .spacing=2,                               \
      }

#define AXES_YLABEL_MAX_LEN 128
#define MAX_OFFSETX 0.9

#define PANX_DECAY 0.6f
#define MINIMUM_PANX 0.05f
#define MAX_PAN_Y 0.5
#define MAX_PAN_X 0.9

#define ZOOMX_DECAY 0.4
#define MINIMUM_ZOOMX 0.2
#define ZOOMX_USE 0.1

static void figure_compile(Figure* figure);
static void figure_create_axes(Figure *figure);
static void load_font(Figure* figure);
static void figure_update_xlim(Figure* figure);

static void insert_bar(bool top, Rect* src, Rect* dest, size_t height, float spacing);
static void figure_draw_title(Figure *figure);
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
  figure->arena=arena;
  return figure;
}

Figure* figure_create(size_t rows, size_t cols)
{
  Figure* figure=figure_create_();
  Fconfig* fc=figure->fc;
  fc->nc=nc_create(NODES_ARENA_SIZE);
  if(!(rows==1&&cols==1))//to allow figure_show to trigger
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
  figure->redraw_count++;
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
  figure->redraw_count++;
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
  if(IsWindowResized())figure->redraw_count++;
}


void figure_update(Figure* figure){
  Fconfig *fc=figure->fc;
  NC* nc=figure->fc->nc;
  float spacing=figure->theme.spacing;
  Rect area=(Rect){0,0,GetScreenWidth(),GetScreenHeight()};
  if(fc->title_bar.is_visible)insert_bar(1, &area,&fc->title_bar.area, figure->theme.font_size,figure->theme.spacing);
  if(fc->mode_bar.is_visible)insert_bar(0, &area,&fc->mode_bar.area, figure->theme.font_size,figure->theme.spacing);
  figure_handle_inputs(figure);
  if(figure->redraw_count||figure->redraw_modebar){
    ClearBackground(figure->theme.bg);
    if(figure->redraw_count){
      nc_set_area(nc,figure->fc->rects,area,spacing);
      figure_update_xlim(figure);
    }
    for(size_t i=0;i<figure->axes_count;++i){
      if(figure->redraw_count)axes_update_ylim(figure->axes+i);
      axes_draw(figure->axes+i);
    }
    figure_draw_cursors(figure);
    figure_draw_title(figure);
    if(figure->redraw_count)figure->redraw_count--;
    if(figure->redraw_modebar)figure->redraw_modebar--;
  }
  figure_mode_bar_draw(figure);
}

void draw_slider(int *value, float min, float max, float x, float y, float w,
                 Font font, Theme theme) {
  float h = 20;
  Vector2 m = GetMousePosition();
  Rectangle bounds = (Rectangle){x, y, w, h};
  bool hover = CheckCollisionPointRec(m, bounds);
  float t = (*value - min) / (max - min);
  if(*value<min)*value=min;
  float knobX = x + t * w;
  if (!hover) {
    DrawRectangleLinesEx(bounds, 1, Fade(DARKGRAY,0.5));
  } else {
    DrawRectangleLinesEx(bounds, 1, LIME);

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
      float nt = (m.x - x) / w;
      if (nt < 0)
        nt = 0;
      if (nt > 1)
        nt = 1;

      *value = min + nt * (max - min);
      knobX = x + nt * w;
    }
  }
  DrawRectangle((int)x, (int)(y + h / 2 - 2), (int)w, 1, YELLOW);
  DrawLine((int)knobX, (int)y+0.1*h, (int)knobX, (int)(y + h*0.8), YELLOW);
  char buf[32];
  snprintf(buf, sizeof(buf), "%d", *value);
  Vector2 size = MeasureTextEx(font, buf, theme.font_size, theme.font_spacing);
  float tx = x + w + 10;
  float ty = y + (h - size.y) * 0.5f;
  DrawTextEx(font, buf, (Vector2){tx, ty}, theme.font_size, theme.font_spacing,theme.fg);
}

void randomize(int *ptr, size_t len, int rand_min, int rand_max){
  ASSERT(ptr&& len>0);
  for(size_t i=0;i<len;++i){
    ptr[i]=(double)rand()/RAND_MAX*(rand_max-rand_min)+rand_min;
  }
}

void figure_show(Figure* figure, char* window_title, int width, int height, int fps)
{
  Fconfig* fc=figure->fc;
  ASSERTM(fc->state==FIGURE_STATE_CREATED,"figure_show called again\n");
  raylib_init(figure,window_title,width,height,fps);//figure state will now be FIGURE_STATE_RUUNING

  srand(time(0));

  while (!WindowShouldClose()) {
    raylib_init_loop(figure);
    BeginDrawing();
    /* ClearBackground(figure->theme.bg); */
    figure_update(figure);
    {
      if(IsKeyPressed(KEY_N)){
        randomize(figure->x,10-1,0,10);
        }
      int slider_count = sizeof(figure->x) / sizeof(figure->x[0]);
      if(figure->x[slider_count-1]){
        Rect area = *figure->axes[figure->axes_count - 1].area;
        float padding = 10.0f;
        float usable_h = area.h - 2 * padding;
        float step = usable_h / slider_count;
        for (int i = 0; i < slider_count; i++) {
          float y = area.y + padding + i * step;
          draw_slider(
                      figure->x + i,
                      1.0f,
                      100.0f,
                      area.x + 30,
                      y,
                      area.w - 100,
                      *figure->font,
                      figure->theme
                      );
        }
      }
    }
    EndDrawing();
    
  }
  CloseWindow();
  fc->state=FIGURE_STATE_STOPPED;
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
  figure->redraw_count++;
}

void figure_set_view_data(Figure* figure, size_t len, char *xformat, double *xdata, size_t timeframe)
{
  ASSERT(figure);
  ASSERT(len>=MINIMUM_VLEN);
  ASSERT(xdata);
  ASSERT(xformat);
  ASSERT(timeframe!=0);
  Fconfig* fc=figure->fc;
  ASSERTM(!fc->vd.len,"view data can only be called once. use update_xdata\n");
  fc->vd.len=len;
  fc->vd.rvlen=fc->vd.vlen=MIN(VIEWPORT_INITIAL_LEN,len);
  fc->vd.zoom.y=-5;
  fc->timeframe=timeframe;
  fc->xdata=arena_alloc(figure->arena,sizeof(*fc->xdata)*len); //always alloc since update_xdata or update_xdatar maybe called elsewhere
  memcpy((fc->xdata=arena_alloc(figure->arena,sizeof(*fc->xdata)*len)),xdata,sizeof(*fc->xdata)*len);
  if(xformat){
    fc->xformat=str_createf_with_arena(figure->arena,"%s", xformat);
  }
}

void figure_update_xdata(Figure* figure, double* xdata, size_t timeframe, size_t start, size_t len)
{
  ASSERT(figure);
  ASSERT(xdata);
  ASSERT(timeframe>0);
  ASSERT(len>0);
  ASSERTM(figure->fc->vd.len,"call figure_set_view_data first\n");
  Fconfig* fc=figure->fc;
  ASSERTM(start+len<=fc->vd.len,"xdata index error; would overflow\n");
  memcpy(fc->xdata+start,xdata,sizeof(*fc->xdata)*len);
  fc->timeframe=timeframe;
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
  figure->font=arena_alloc(figure->arena,sizeof(Font));
  Font* font=figure->font,temp;
  *font=GetFontDefault();
  if(fc->font_path){
    temp=LoadFont(fc->font_path);
    if(temp.baseSize!=0){
      *font=temp;
    }
    str_free(fc->font_path);
  }
}

static void figure_update_xlim(Figure* figure)
{
  Fconfig *fc=figure->fc;
  if(!fc->xdata)return;
  figure->xlim=get_lim(fc->xdata+fc->vd.start,fc->vd.rvlen);
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
  Theme theme=figure->theme;
  DrawRectangle(NODE_RECT_UNWRAP(area),theme.bg);
  Vd vd=fc->vd;
#define LEN 256
  char buf_[LEN]={0};
  char* buf=str_create(buf_,LEN);
  str_clear(buf);
  str_append(buf,"fps=%d ",GetFPS());
  Axes *aum=get_axes_under_mouse(figure);
  if(fc->xdata && aum && aum->artist_len){
    float mx=(GetMouseX()-aum->area->x)/aum->area->w;
    float index=mx*vd.rvlen+vd.start+fmin(vd.offsetx,0.f);
    if(index>=0.f && index<vd.len){
      str_append(buf,"{start:'%'zu',at:'%'zu',showing:'%'zu', all:'%'zu'} ",vd.start,(size_t)index,vd.vlen,vd.len);
      size_t epoch= fc->xdata[(int)index]+(index-floor(index))*fc->timeframe;
      epoch2strftime(epoch,buf, fc->xformat);
    }
  }
  float x=align_text(figure->font,buf,area.w, theme.font_size,theme.spacing, ALIGNMENT_LEFT);
  DrawTextEx(*figure->font,buf,(Vector2){x+area.x,area.y+1},theme.font_size,theme.font_spacing,theme.fg);
  if(fc->xdata){
    str_clear(buf);
    str_append(buf,"%s","xindex[-1]=");
    epoch2strftime(fc->xdata[vd.len-1],buf, fc->xformat);
    x=align_text(figure->font,buf,area.w, theme.font_size,theme.spacing, ALIGNMENT_RIGHT);
    DrawTextEx(*figure->font,buf,(Vector2){x+area.x,area.y+1},theme.font_size,theme.font_spacing,theme.fg);
  }

}

static void figure_handle_inputs(Figure* figure)
{
  Fconfig *fc=figure->fc;
  Vd vd=fc->vd;
  Vector2 md=GetMouseDelta();
  Axes* aum=get_axes_under_mouse(figure);
  int has_artists=aum?aum->artist_len>0:0;
  int mbd=IsMouseButtonDown(MOUSE_BUTTON_LEFT);
  int need_update=0;
  figure->redraw_modebar+=(int)(md.x!=0.f|| md.y!=0);
  
  if(vd.aum!=aum){
    vd.aum=aum;
    vd.wait_mouse_up=true;
  }
  if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
    vd.wait_mouse_up=false;
    vd.panx_velocity=0;
  }
  if(fabsf(md.x)>fabsf(md.y))md.y=0;
  else md.x=0;
  if(!vd.wait_mouse_up && has_artists && mbd){
    if(md.x!=0) vd.panx_velocity-=(md.x/aum->area->w);
  }

  //panning with mouse horiz
  if(vd.panx_velocity!=0.f && has_artists){
    float x=MAX_OFFSETX*vd.rvlen;
    vd.rstart=fmin(fmax(-x,(vd.panx_velocity)*vd.rvlen+vd.rstart),vd.len-vd.rvlen+x);
    vd.offsetx=vd.rstart<0.f?vd.rstart:vd.rstart+vd.rvlen>=vd.len?vd.rstart+vd.rvlen-vd.len:vd.rstart-floor(vd.rstart);
    need_update=1;
  }

  if(vd.panx_velocity!=0.f){
    vd.panx_velocity*=PANX_DECAY;
    if(fabsf(vd.panx_velocity)<MINIMUM_PANX)vd.panx_velocity=0.f;
  }
  //fast move horiz with ARROW_(LEFT/RIGHT)
  int arrow_key=(IsKeyPressed(KEY_LEFT)||IsKeyDown(KEY_LEFT))?-1.0:
    (IsKeyPressed(KEY_RIGHT)||IsKeyDown(KEY_RIGHT))?1.0:0;
  if(arrow_key!=0){
    vd.panx_velocity+=arrow_key*ARROW_KEYS_SCALE;
    need_update=1;
  }

  //move to the far left or right with KEY_A/KEY_Z
  int where=IsKeyPressed(KEY_A)?-1:IsKeyPressed(KEY_Z)?1:0;
  if(where){
    float offsetx=MAX_OFFSETX*0.05*vd.rvlen;
    vd.rstart=where<0?-offsetx:vd.len-vd.rvlen+offsetx;
    vd.offsetx=offsetx*where;
    need_update=1;
  }
    
  //dragging with mouse vert
  if(!vd.wait_mouse_up   &&has_artists &&mbd){;
    vd.offsety+=(md.y/aum->area->h);;
    float oy=fabs(vd.offsety);
    if(oy>MAX_PAN_Y)vd.offsety=oy/vd.offsety*MAX_PAN_Y;
    if (md.y!=0.f) need_update=1;
  }

  Vector2 wm=GetMouseWheelMoveV();
  //zoomx
  if(wm.y!=0.f)vd.zoomx_velocity=wm.y*ZOOMX_USE*vd.rvlen;
  if(aum && vd.zoomx_velocity!=0.f){
    float left=(GetMouseX()-aum->area->x)/aum->area->w;
    double vlen=fmin(vd.len,fmax(MINIMUM_VLEN,vd.rvlen-vd.zoomx_velocity));
    vd.rstart=fmin(vd.len-vlen,fmax(0,(vd.rstart+vd.zoomx_velocity*left)));
    vd.rvlen=vlen;
    vd.offsetx=vd.rstart<0.f?vd.rstart:vd.rstart+vd.rvlen>=vd.len?vd.rstart+vd.rvlen-vd.len:vd.rstart-floor(vd.rstart);
    vd.zoomx_velocity*=ZOOMX_DECAY;
    if(fabsf(vd.zoomx_velocity)<MINIMUM_ZOOMX||
       vd.rvlen==MINIMUM_VLEN||
       vd.rvlen==vd.len)vd.zoomx_velocity=0.f;
    need_update=1;
  }


  //zoomy
  if(wm.x!=0.f){
    float temp=vd.zoom.y;
    vd.zoom.y+=wm.x*ZOOMY_SCALE;
    vd.zoom.y=MAX(-MAX_ZOOMY,MIN(MAX_ZOOMY,vd.zoom.y));
    if(temp!=vd.zoom.y)need_update=1;
  }

  //reset with KEY_R
  if(IsKeyPressed(KEY_R)){
    vd.panx_velocity=0.f;
    vd.offsety=0.f;
    vd.zoom.y=-5;  
    need_update=1;    
  }

  vd.start=vd.offsetx<0.f?0:vd.offsetx<1.f?vd.rstart:ceil(vd.rstart);
  vd.vlen=vd.offsetx<0.f?vd.rvlen-fabsf(vd.offsetx):vd.offsetx<1.f?vd.rvlen:vd.rvlen-vd.offsetx;

  vd.draw_len=vd.vlen;
  for(size_t i=0;i<2;++i){
    if(vd.draw_len+vd.rstart<vd.len-1)vd.draw_len+=1;
    else break;
  }
  
  if(need_update)figure->redraw_count++;
  fc->vd=vd;
}


static void  figure_draw_title(Figure *figure)
{
  Fconfig *fc=figure->fc;
  if(!fc->title_bar.is_visible || !figure->title)return;
  Theme theme=figure->theme;
  Rect area=fc->title_bar.area;
  float x=align_text(figure->font,figure->title,area.w, theme.font_size,theme.spacing, ALIGNMENT_CENTER);
  DrawTextEx(*figure->font,figure->title,(Vector2){x+area.x,area.y+1},theme.font_size,theme.font_spacing,theme.fg);
}


static void  figure_draw_cursors(Figure *figure)
{
  if(!figure->fc->show_cursors)return;
  Theme theme=figure->theme;
  Vector2 mp=GetMousePosition();
  Axes* aum=get_axes_under_mouse(figure);
  if(!aum||!aum->artist_len)return;
  Rect area=*aum->area;
  double mreal=scale_to_limitf(mp.y, (Limit){area.y+area.h,area.y},aum->lim);
  float px=scale_to_limitf(mp.x, (Limit){area.x,area.x+area.w},(Limit){0.f, 1.f});
 Color c=Fade(theme.fg,0.5);
  for(size_t i=0;i<figure->axes_count;++i){
    Axes axes=figure->axes[i];
    if(!axes.artist_len)continue;
    Rect areax=*axes.area;
    float endy=areax.y+areax.h;
    float pixely=scale_to_limitf(mreal,axes.lim,(Limit){endy,areax.y});
    int pxv=px*areax.w+areax.x;
    DrawLine(pxv,areax.y,pxv,areax.y+areax.h,c);
    if(pixely<areax.y || pixely>endy)continue;
    DrawLine(areax.x,pixely,areax.x+areax.w,pixely,c);
  }
}


static void figure_axes_init(Figure* figure)
{
  const int buflen=256;
  char buf_[buflen];
  Theme theme=figure->theme;
  Font font=*figure->font;
  char* buf=str_create(buf_,buflen);
  for(size_t i=0;i<figure->axes_count;++i){
    Axes* axes=figure->axes+i;
    if(!axes->artist_len)continue;
    str_clear(buf);
    for(size_t x=0;x<axes->legend.llen;++x)str_append(buf,"%s"," ");
    axes->legend.llen=MeasureTextEx(font, buf, theme.font_size,theme.font_spacing).x;
  }
}

