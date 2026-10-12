#include "view.h"
#include <string.h>
#include "raycandle.h"

BarSeries* bar_series_create(size_t timeframe, size_t len, Candles candles, double* epochs, Arena *arena, const char *title)
{
  ASSERT(timeframe>0);
  ASSERTM(len>=2,"atleast 2 records are needed\n");
  ASSERT(epochs &&arena);
  BarSeries *bs=arena_alloc(arena,sizeof(BarSeries));
  memset(bs,0,sizeof(BarSeries));
  bs->timeframe=timeframe;
  bs->len=len;
  bs->epochs=arena_alloc(arena,sizeof(*bs->epochs)*bs->len);
  memcpy(bs->epochs,epochs,sizeof(*bs->epochs)*bs->len);
  Candles c={0};
  c.o=arena_alloc(arena, sizeof(*c.o)*bs->len*4);
  c.h=c.o+bs->len*1;
  c.l=c.o+bs->len*2;
  c.c=c.o+bs->len*3;
  bs->candles=c;
  memcpy(c.o,candles.o,sizeof(*c.o)*bs->len);
  memcpy(c.h,candles.h,sizeof(*c.o)*bs->len);
  memcpy(c.l,candles.l,sizeof(*c.o)*bs->len);
  memcpy(c.c,candles.c,sizeof(*c.o)*bs->len);
  bs->ur=UPDATE_REASON_INIT;
  bs->curr_epoch=epochs[len-1]-(size_t)epochs[len-1]%timeframe;
  if(title)bs->title=str_createf_with_arena(arena,"%s",title);
  return bs;
}
  

void bar_series_update(BarSeries *bs, double epoch, float price)
{
  size_t lm1=bs->len-1;
  size_t iepoch=epoch;
  size_t curr_anchor=iepoch - iepoch%bs->timeframe;
  Candles c=bs->candles;
  ASSERTM(bs->epochs[lm1]<=epoch, "cannot update with a past epoch past: %f new %f\n",bs->epochs[lm1],epoch);
  if(curr_anchor!=bs->curr_epoch){
    bs->epochs[lm1]=curr_anchor;
    for(size_t i=0;i<lm1;++i){
      c.o[i]=c.o[i+1];
      c.h[i]=c.h[i+1];
      c.l[i]=c.l[i+1];
      c.c[i]=c.c[i+1];
      bs->epochs[i]=bs->epochs[i+1];
    }
    c.o[lm1]=c.h[lm1]=c.l[lm1]=c.c[lm1]=price;
    bs->epochs[lm1]=epoch;
    bs->ur=UPDATE_REASON_CANDLE;
    bs->curr_epoch=curr_anchor;
  }
  else{
    if(c.h[lm1]<price)c.h[lm1]=price;
    if(c.l[lm1]>price)c.l[lm1]=price;
    c.c[lm1]=price;
    bs->epochs[lm1]=iepoch;
    bs->ur=UPDATE_REASON_TICK;
  }
}

