#include "mouse_updater.h"
#include "axes.h"
#include <math.h>
#include "figure.h"
#include "artist.h"
#include <assert.h>
#include <stdbool.h>
#include "raycandle.h"
#include "utils.h"
#include <string.h>

static void update_from_diffx(float diffx,Figure* figure);
static void update_ylim_not_static(Axes* axes);


void zoomx(Figure* figure,int move){
  figure->zoomx_padding+=move/100.f;
  figure->zoomx_padding=figure->zoomx_padding>-0.5?figure->zoomx_padding:0;
  update_from_position(figure->dragger.start,figure);
}

void zoomy(Figure* figure,int move){
  RC_ASSERT(RC_ZOOMY_SCALE>=1);
  long int vlen,start,temp;
  start=figure->dragger.start;
  vlen=figure->dragger.vlen;
  temp=RC_MAX_PLOTTABLE_LEN>figure->dragger._len?figure->dragger._len:RC_MAX_PLOTTABLE_LEN;
#define ZOOMING_IN move==-1
  if(ZOOMING_IN){
    vlen=vlen==temp?vlen:minl(temp,vlen+2*RC_ZOOMY_SCALE);
    start=vlen==temp?start:maxl(0,start-RC_ZOOMY_SCALE);
    if(start+vlen>(long int)figure->dragger._len)start=figure->dragger._len-vlen;
  }
  else
    {
      vlen=vlen==1?1:maxl(1,vlen-2*RC_ZOOMY_SCALE);
      start=vlen==1?start:start+RC_ZOOMY_SCALE;
      if(start+vlen>(long int)figure->dragger._len)start=figure->dragger._len-1;
    }
  figure->dragger.start=start;
  figure->dragger.vlen=vlen;
  update_from_position(figure->dragger.start,figure);
}

void mouse_updates(Figure *figure){
  RC_ASSERT(figure->dragger.ulen>0,"cannot update if upate len is 0\n");
  Vector2 mouse;
  mouse= GetMousePosition();
  MouseDrag* mouse_drag=&figure->mouse_drag;
  Axes* axes=get_axes_under_mouse(figure);
  /* 1.Pressing either left or right arrow For faster Movements*/
  int bt=(IsKeyPressed(KEY_LEFT)||IsKeyDown(KEY_LEFT)?1:IsKeyPressed(KEY_RIGHT)||IsKeyDown(KEY_RIGHT)?-1:0)*10;
  if(bt!=0){
    update_from_diffx(bt, figure);
    return;
  }
  /* 2.wheel move*/
  if(axes&&axes->artist_len!=0){
    Vector2 v=GetMouseWheelMoveV();
    if(v.y==0&&v.x!=0){return zoomx(figure,v.x);}
    if(v.x==0&&v.y!=0){return zoomy(figure,v.y);}
  }

  /*3.mouse horizontal and vertical drags*/
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)&&!mouse_drag->dragging&&axes&&axes->artist_len!=0)
    {
      mouse_drag->dragging=true;
      mouse_drag->axes=axes;
    }      
  else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
    memset(mouse_drag, 0, sizeof(*mouse_drag));
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
  }
  if (mouse_drag->dragging){
    if(axes!=mouse_drag->axes){//dragged outside this axes
      SetMouseCursor(MOUSE_CURSOR_DEFAULT);
      memset(mouse_drag, 0, sizeof(*mouse_drag));      
    }
    else{
      mouse_drag->posx.y=mouse.x-mouse_drag->posx.x;
      mouse_drag->posy.y=mouse.y-mouse_drag->posy.x;
      if(mouse_drag->posx.x!=0||mouse_drag->posy.x!=0){
        bool horiz=fabs(mouse_drag->posx.y)>fabsf(mouse_drag->posy.y);
        float ratio=horiz?mouse_drag->posx.y:mouse_drag->posy.y;
        if(horiz&&mouse_drag->posx.x!=0){//horizontal
          SetMouseCursor(MOUSE_CURSOR_RESIZE_EW);
          update_from_diffx(ratio/axes->width*axes->parent->dragger.vlen,figure);
        }
        if(!horiz&&ratio!=0.f){//horizontal{//vertical
          SetMouseCursor(MOUSE_CURSOR_RESIZE_NS);
          figure->vertical_limit_drag+=ratio/axes->height;
          axes->parent->force_update=true;
        }
      }
      mouse_drag->posx.x=mouse.x;
      mouse_drag->posy.x=mouse.y;
    }
  }

  /**
     4. Pressing l or h to move forward  or backward 1 update respectively
  */

  if(IsKeyPressed(KEY_L)){
    if(figure->dragger.start+figure->dragger.vlen+1<=figure->dragger._len){update_from_position(figure->dragger.start+1,figure);}
    return;
  }
  if(IsKeyPressed(KEY_H)){
    if(figure->dragger.start>0){update_from_position(figure->dragger.start-1,figure);}
    return;
  }

  //Reset
  if(IsKeyPressed(KEY_R)){
    long int start;
    float ratio=axes?(float)(mouse.x-axes->startX)/axes->width:0.5f;
    start=figure->dragger.start+(figure->dragger.vlen*ratio)-(RC_INITIAL_VISIBLE_DATA*ratio);
    figure->vertical_limit_drag=figure->zoomx_padding=0;
    figure->dragger.vlen=RC_INITIAL_VISIBLE_DATA;    
    start=minl(maxl(start,0),figure->dragger._len-figure->dragger.vlen);
    figure->dragger.start=start;
    figure->force_update=true;
  }
}

/**
   convert the diff between last mousex position and the current to visible data the update the 
   figure
*/
static void update_from_diffx(float diffx,Figure* figure){
  long int start=diffx*-1*figure->dragger.ulen+figure->dragger.start;
  start=start<0?0:start+(long int)figure->dragger.vlen>(long int)figure->dragger._len?(long int)figure->dragger._len-(long int)figure->dragger.vlen:start;
  if((size_t)start==figure->dragger.start){
    return;
  }
  update_from_position((size_t)start,figure);
}


static void update_ylim_not_static(Axes *axes) {
  RC_ASSERT(!axes->ylocator.limit.is_static);
  double lmax = NAN, lmin = NAN, value;
  size_t start=axes->parent->dragger.start;
  for (size_t a = 0; a < axes->artist_len; ++a) {
    Artist artist = *get_artist(axes, a);
    if (!artist.ylim_consider)continue;
    RC_ASSERT(artist.gdata.ydata != NULL && artist.gdata.cols > 0);
    for (size_t i = 0; i < artist.gdata.cols; ++i) {
      double *ydata = &(*artist.gdata.ydata) + (i * axes->parent->dragger._len);
      for (size_t s = start; s < axes->parent->dragger.vlen+start; s++) {
        value = ydata[s];
        if (!isfinite(value)) {
          continue;
        }
        lmax = fmax(lmax, value);
        lmin = fmin(lmin, value);
      }
    }
  }
  float diff = lmax - lmin;
  float vertical_limit_drag=diff * axes->parent->vertical_limit_drag;
  lmax+=vertical_limit_drag;
  lmin+=vertical_limit_drag;
  float dadd = diff * axes->parent->zoomx_padding;
  diff += dadd * 2;
  lmax +=dadd;
  lmin -= dadd;
  axes->ylocator.limit = (Limit){.limit_max = lmax, .limit_min = lmin, .diff = diff};
}

#define BUF_LEN 128
void measure_ylabel(Axes* axes){
  char _buffer[BUF_LEN]={0};
  Str string=string_create(BUF_LEN,_buffer);
  string_append(string,"    %.5f ",axes->ylocator.limit.limit_min);
  axes->ylabel_len=MeasureTextEx(*(Font*)axes->parent->label_font,string,RC_LABEL_FONT_SIZE,axes->parent->font_spacing).x;
  string_clear(string);
  string_append(string,"    ");
  axes->ylabel_padding=MeasureTextEx(*(Font*)axes->parent->label_font,string,RC_LABEL_FONT_SIZE,axes->parent->font_spacing).x;
  string_destroy(string);
}
#undef BUF_LEN


void update_from_position(size_t start,Figure* figure){
  RC_ASSERT(figure->has_dragger);
  RC_ASSERT(start<=figure->dragger._len);
  RC_ASSERT(start+figure->dragger.vlen<=figure->dragger._len);
  figure->dragger.start=start;
  update_xlim(figure);
  for(size_t i=0;i<figure->axes_len;++i){
    if(figure->axes[i].artist_len==0){continue;}
    //if ylocator limit is not static i:e user has not called set_ylim, we update y-axis automatically
    if(!figure->axes[i].ylocator.limit.is_static){update_ylim_not_static(figure->axes+i);}
    measure_ylabel(&figure->axes[i]);
    locator_update_data_buffers(figure->axes+i, LIMIT_CHANGED_ALL_LIM);

  }
}
