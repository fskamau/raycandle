#include "mouse_updater.h"
#include "axes.h"
#include <math.h>
#include "figure.h"
#include "artist.h"
#include <assert.h>
#include <stdbool.h>
#include "raycandle.h"
#include "utils.h"
static void update_from_diffx(int diffx,Figure* figure);
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
#define ZOOMING_IN move==1
  if(ZOOMING_IN){
    vlen=vlen==temp?vlen:RC_MIN(temp,vlen+2*RC_ZOOMY_SCALE);
    start=vlen==temp?start:RC_MAX(0,start-RC_ZOOMY_SCALE);
    if(start+vlen>(long int)figure->dragger._len)start=figure->dragger._len-vlen;
  }
  else
    {
      vlen=vlen==1?1:RC_MAX(1,vlen-2*RC_ZOOMY_SCALE);
      start=vlen==1?start:start+RC_ZOOMY_SCALE;
      if(start+vlen>(long int)figure->dragger._len)start=figure->dragger._len-1;
    }
  figure->dragger.start=start;
  figure->dragger.vlen=vlen;
  update_from_position(figure->dragger.start,figure);
}

void mouse_updates(Figure *figure){
  RC_ASSERT(figure->dragger.ulen>0);
  MouseInfo* mouseinfo=&figure->mouseinfo;    
  Axes * axes=mouseinfo->axes;
  int mousex=GetMouseX();
  uint8_t aium=get_axes_index_under_mouse(figure);
 /* 1.Pressing either left or right arrow For faster Movements*/
  int bt=(IsKeyPressed(KEY_LEFT)||IsKeyDown(KEY_LEFT)?1:IsKeyPressed(KEY_RIGHT)||IsKeyDown(KEY_RIGHT)?-1:0)*10;
  if(bt!=0){
    update_from_diffx(bt, figure);
    return;
  }
  /* 2.wheel move*/
  if(aium!=figure->axes_len&&figure->axes[aium].artist_len!=0){
    Vector2 v=GetMouseWheelMoveV();
    if(v.y==0&&v.x!=0){return zoomx(figure,v.x);}
    if(v.x==0&&v.y!=0){return zoomy(figure,v.y);}
  }

  /*3.mouse click left and drag*/
  if(mouseinfo->wait_up){if((mouseinfo->wait_up=IsMouseButtonDown(MOUSE_LEFT_BUTTON))){return;}}
  if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)){
    if(aium==figure->axes_len || figure->axes[aium].artist_len==0){
    invalid_mouse_position:
      SetMouseCursor(MOUSE_CURSOR_DEFAULT);
      mouseinfo->wait_up=true;
      mouseinfo->down=false;
      return;
    }
    if(mouseinfo->down){
      if(figure->axes[aium].label!=mouseinfo->axes->label){
        goto invalid_mouse_position;
      }
      if((int)mousex!=mouseinfo->diffx){
	  mouseinfo->accumulator+=((float)mousex-mouseinfo->diffx)/mouseinfo->axes->width*axes->parent->dragger.vlen*axes->parent->dragger.timeframe;
	  if((size_t)fabsf(mouseinfo->accumulator)>figure->dragger.timeframe){
	    update_from_diffx((mouseinfo->accumulator>0.f?0:1)+(int)floorf(mouseinfo->accumulator/figure->dragger.timeframe),figure);
	    mouseinfo->accumulator=0.f;
	  }       	
      }
      mouseinfo->diffx=mousex;
    }
    else{
      mouseinfo->down=true;
      mouseinfo->axes=axes=figure->axes+aium;
      SetMouseCursor(MOUSE_CURSOR_RESIZE_EW);
      mouseinfo->diffx=mousex;
      mouseinfo->accumulator=0.f;
    }
  }
  else{
    mouseinfo->down=false;
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
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
}

/**
   convert the diff between last mousex position and the current to visible data the update the 
   figure
*/
static void update_from_diffx(int diffx,Figure* figure){
  long int start=diffx*-1*figure->dragger.ulen+figure->dragger.start;
  start=start<0?0:start+(long int)figure->dragger.vlen>(long int)figure->dragger._len?(long int)figure->dragger._len-(long int)figure->dragger.vlen:start;
  if((size_t)start==figure->dragger.start){return;}
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
          lmax = RC_MAX(lmax, value);
          lmin = RC_MIN(lmin, value);
        }
      }
  }
  float diff = lmax - lmin;
  float dadd = diff * axes->parent->zoomx_padding;
  diff += dadd * 2;
  lmax += dadd;
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
