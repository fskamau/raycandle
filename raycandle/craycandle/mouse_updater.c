#include "mouse_updater.h"
#include "axes.h"
#include <math.h>
#include "figure.h"
#include "artist.h"
#include <assert.h>

static void update_from_diffx(int diffx,Figure* figure);
static void update_ylim_not_static(Axes* axes);
#define LOCATOR_NOT_STATIC_YLIM_PERCENTAGE_ADDED (2.f/100)



void mouse_updates(Figure *figure){
  RC_ASSERT(figure->dragger.update_len>0);
  MouseInfo* mouseinfo=&figure->mouseinfo;    
  Axes * axes=mouseinfo->axes;
  
 /* 1.Pressing either left or right arrow For faster Movements*/
  int bt=(IsKeyPressed(KEY_LEFT)||IsKeyDown(KEY_LEFT)?1:IsKeyPressed(KEY_RIGHT)||IsKeyDown(KEY_RIGHT)?-1:0)*10;
  if(bt!=0){
    update_from_diffx(bt, figure);
    return;
  }
  /* 2.wheel move*/
  Vector2 v=GetMouseWheelMoveV();
  if(v.y==0&&v.x!=0){
    update_from_diffx(10*v.x, figure); // multiplied to increase sensitivity
    return;
  }

  /*3.mouse click left and drag*/
  int mousex=GetMouseX();
  if(mouseinfo->wait_up){if((mouseinfo->wait_up=IsMouseButtonDown(MOUSE_LEFT_BUTTON))){return;}}
  if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)){
    uint8_t aium=get_axes_index_under_mouse(figure);
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
	  mouseinfo->accumulator+=((float)mousex-mouseinfo->diffx)/mouseinfo->axes->width*figure->dragger.xlimit.diff;
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
    if(figure->dragger.cur_position+1<=figure->dragger.len_data){update_from_position(figure->dragger.cur_position+1,figure);
    }
    return;
  }
  if(IsKeyPressed(KEY_H)){
    if(figure->dragger.cur_position-1>=figure->dragger.visible_data){update_from_position(figure->dragger.cur_position-1, figure);}
    return;
  }
}

/**
   convert the diff between last mousex position and the current to visible data the update the 
   figure
*/
static void update_from_diffx( int diffx,Figure* figure){
  diffx*=(figure->dragger.update_len);
  size_t new_position=RC_MIN(RC_MAX(figure->dragger.visible_data,(signed long int)figure->dragger.cur_position>=diffx?figure->dragger.cur_position-diffx:0),figure->dragger.len_data);
  if(new_position==figure->dragger.cur_position){return;}
  update_from_position(new_position,figure);
}


void update_from_position(size_t new_position,Figure* figure){
  RC_ASSERT(figure->has_dragger);
  RC_ASSERT(figure->dragger.visible_data<=new_position && new_position<=figure->dragger.len_data);  
  figure->dragger.cur_position=new_position;
  figure->dragger.start=new_position-figure->dragger.visible_data;
  update_xlim_shared(figure);
  for(size_t i=0;i<figure->axes_len;++i){
    if(figure->axes[i].artist_len==0){continue;}
    //if ylocator limit is not static i:e user has not called set_ylim, we update y-axis automatically
    if(!figure->axes[i].ylocator.limit.is_static){update_ylim_not_static(figure->axes+i);}
    locator_update_data_buffers(figure->axes+i, LIMIT_CHANGED_ALL_LIM);
  }
}

static void update_ylim_not_static(Axes *axes) {
  RC_ASSERT(!axes->ylocator.limit.is_static);
  double lmax = NAN, lmin = NAN, value;
  for (size_t a = 0; a < axes->artist_len; ++a) {
    Artist artist = *get_artist(axes, a);
    if (!artist.ylim_consider)continue;
      RC_ASSERT(artist.gdata.ydata != NULL && artist.gdata.cols > 0);
      for (size_t i = 0; i < artist.gdata.cols; ++i) {
        double *ydata = &(*artist.gdata.ydata) + (i * axes->parent->dragger.len_data);
        for (size_t s = axes->parent->dragger.start; s < axes->parent->dragger.cur_position; s++) {
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
  float dadd = diff * LOCATOR_NOT_STATIC_YLIM_PERCENTAGE_ADDED;
  diff += dadd * 2;
  lmax += dadd;
  lmin -= dadd;
  axes->ylocator.limit = (Limit){.limit_max = lmax, .limit_min = lmin, .diff = diff};
}
