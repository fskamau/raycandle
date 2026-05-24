#include "node.h"
#include "raycandle.h"
#include <string.h>
#include "figure.h"
#include "artist.h"
#include <math.h>
#include "artist.h"



void axes_draw(Axes* axes){
  Rect area=*axes->area;
  DrawRectangleLines(NODE_RECT_UNWRAP(area),axes->fig->theme.fg);
  Artist* artist=axes->artist;
  if(!artist)return;
  if(axes->fig->force_update)
    {
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
      axes->lim=lim;
    }
  artist=axes->artist;
  while(artist){
    artist->avt->draw(artist,axes,area);
    artist=artist->next;
    
  }
}
