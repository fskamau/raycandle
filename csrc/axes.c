#include "node.h"
#include "raycandle.h"
#include <string.h>
#include "figure.h"
#include "artist.h"
#include <math.h>
#include "artist.h"

#define YLABEL_STROKE_SIZE 5
#define YLABEL_PADDING 10
#define YLABEL_GAP 5

static void axes_set_ylabel_len(Axes* axes);
static void axes_draw_ylabel(Axes* axes);

void axes_draw(Axes* axes){
  Vd vd=axes->fig->fc->vd;        
  Rect *area=axes->area;

  {
    DrawRectangleLines(NODE_RECT_UNWRAP(*area),axes->fig->theme.fg);
    area->x+=1;
    area->y+=1;
    area->w-=2;
    area->h-=2;  
  }
  axes_set_ylabel_len(axes);
  Artist* artist=axes->artist;
  if(!artist)return;

  area->x-=vd.offsetx*axes->w;
  area->w+=vd.offsetx*axes->w;
  artist=axes->artist;
  
  BeginScissorMode(NODE_RECT_UNWRAP(*area));
  while(artist){
    artist->avt->draw(artist,axes,*area);
    artist=artist->next;    
  }
  EndScissorMode();
  axes_draw_ylabel(axes);
}

void axes_update_limy(Axes* axes)
{
  Artist* artist=axes->artist;
  if(!artist)return;
  Rect area=*axes->area;
  Limit lim=(Limit){NAN,NAN};
  Vd vd=axes->fig->fc->vd;        
  ASSERT(vd.vlen!=0);
  axes->w=area.w/(vd.vlen);
  while(artist&&artist->is_limitable){
    Limit tlim=artist->avt->get_limit(artist,axes);
    if (!isnan(tlim.min)) {
      if (isnan(lim.min)) {
        lim.min = tlim.min;
      } else if (tlim.min < lim.min) {
        lim.min = tlim.min;
      }
    }
    
    if (!isnan(tlim.max)) {
      if (isnan(lim.max)) {
        lim.max = tlim.max;
      } else if (tlim.max > lim.max) {
        lim.max = tlim.max;
      }
    }
    artist=artist->next;  
  }
  /* add offsetY to the limit */
  if(vd.offsety!=0.f){
    double diff=(lim.max-lim.min)*vd.offsety;
    lim.min+=diff;
    lim.max+=diff;
  }
  axes->lim=lim;
}


Axes* get_axes_under_mouse(Figure* figure)
{
  for(size_t i=0;i<figure->fc ->axes_count;++i){
    if(CheckCollisionPointRec(GetMousePosition(),NODE_RECT2RECTANGLE(figure->fc->rects[i])))
      return figure->axes+i;
  }
  return NULL;
}


static void axes_set_ylabel_len(Axes* axes)
{
  if(!axes->artist_len)return;;
  Theme theme=axes->fig->theme;
  Font font=axes->fig->fc->font;
  #define LEN 128
  char buf_[LEN]={0};
  char *buf=str_create(buf_,LEN);
  Limit lim=axes->lim;
  Rect *area=&axes->ylabel.area;
  
  str_clear(buf);
  str_append(buf,axes->ylabel.format,lim.min);
  size_t a=MeasureTextEx(font,buf,theme.font_size,theme.font_spacing).x;

  str_clear(buf);
  str_append(buf,axes->ylabel.format,lim.max);
  area->w=MeasureTextEx(font,buf,theme.font_size,theme.font_spacing).x;

  area->w=MAX(a,area->w);
  area->w=YLABEL_STROKE_SIZE+YLABEL_GAP+area->w+YLABEL_GAP;

  area->x=axes->area->x+axes->area->w-area->w;
  area->y=axes->area->y;
  area->h=axes->area->h;
  
  axes->area->w-=(area->w);
}

void axes_set_yformatter(Axes* axes,char* format)
{
  ASSERT(axes&& format);
  str_clear(axes->ylabel.format);
  str_append(axes->ylabel.format,"%s",format);
}



static void axes_draw_ylabel(Axes* axes)
{
  if(!axes->artist_len)return;
  Rect area=axes->ylabel.area;
  Theme theme=axes->fig->theme;
  Font font=axes->fig->fc->font;
  size_t label_count=area.h/(theme.font_size+YLABEL_PADDING);
  if(label_count==0)return;

  int aum=get_axes_under_mouse(axes->fig)==axes;
  int mousey=GetMouseY(); 

  float half_font=theme.font_size/2.f; ;
  
#define LEN 128
  char buf_[LEN]={0};
  char* buf=str_create(buf_, LEN);
  Limit srcl=(Limit){axes->area->y,axes->area->y+axes->area->h};
  float startx=area.x+YLABEL_STROKE_SIZE+YLABEL_GAP;
  
  for(size_t i=0;i<label_count;++i){
    float y=area.y+i*(YLABEL_PADDING+theme.font_size)+YLABEL_PADDING;
    int diffm=mousey-y;
    if(diffm >0&& diffm<theme.font_size+half_font)continue;
    DrawLine(area.x,y+half_font,area.x+YLABEL_STROKE_SIZE,y+half_font,Fade(theme.fg,0.8));
    double value=scale_to_limitf(y+half_font,srcl,axes->lim);
    str_clear(buf);
    str_append(buf,axes->ylabel.format,value);
    DrawTextEx(font, buf, (Vector2){startx,y},theme.font_size,theme.font_spacing,theme.fg);
  }

  //draw the cursor data
  if(!aum)return;
  double value=scale_to_limitf(mousey,srcl,axes->lim);
  str_clear(buf);
  str_append(buf,axes->ylabel.format,value);
  DrawLine(area.x,mousey,area.x+YLABEL_STROKE_SIZE+5,mousey,theme.fg);
  DrawTextEx(font, buf, (Vector2){startx,mousey-half_font},theme.font_size,theme.font_spacing,RED);
}

