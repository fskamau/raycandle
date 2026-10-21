#include "raycandle.h"
#include "figure.h"
#include "utils.h"
#include <string.h>
#include "artist.h"
#include <math.h>
#include "utils.h"
#include "axes.h"

typedef struct Artist Artist;
static Artist*   get_last_artist(Artist* artist);



#define DEFINE_AVT_FUNCS(a)                                        \
  static void  artist_##a##_init(Artist* artist);                  \
  static Limit artist_##a##_get_limit(Artist* artist);             \
  static void  artist_##a##_draw(Artist* artist, Rect area);       \
  static void  artist_##a##_draw_icon(Artist* artist, Vector2 start_pos)

DEFINE_AVT_FUNCS(line);
DEFINE_AVT_FUNCS(candle);

#undef DEFINE_AVT_FUNCS

#define CREATE_AVT(n) static ArtistVTable n##ATV=(ArtistVTable){    \
    .init=artist_##n##_init,                                        \
    .get_limit=artist_##n##_get_limit,                              \
    .draw=artist_##n##_draw,                                        \
    .draw_icon=artist_##n##_draw_icon}


CREATE_AVT(line);
CREATE_AVT(candle);

#undef CREATE_AVT

/*
  creates an artist
  artist->avt should deal with nullable *colors and *data
*/


Artist* artist_create(Axes *axes, ArtistType atype, float thick, Color* colors, const char* label, void *data)
{
  ASSERT(axes);
  ASSERT(axes->fig->fc->state==FIGURE_STATE_CREATED);
  ASSERTM(axes->fig->fv->vd.len >= 2,"initialize the active view before plotting\n");
  ASSERTM(!axes->fig->fc->figure_view_locked, "cannot create artists after calling figure_view_append\n");
  Artist* artist = arena_alloc(axes->fig->arena,sizeof(Artist));
  memset(artist, 0,sizeof(Artist));
  artist->atype=atype;
  artist->thick=thick;
  artist->data=data;
  artist->view=axes->fig->fv;
  artist->is_limitable=true;
  artist->colors=colors;
  artist->axes=axes;
  if(label){
      artist->label=str_createf_with_arena(axes->fig->arena, "%s", label);
      size_t x=str_len(artist->label);
      axes->av->legend.llen=MAX(axes->av->legend.llen,x);
    }
  switch(atype){
  case ARTIST_TYPE_LINE:
    artist->avt=&lineATV;
    break;
  case ARTIST_TYPE_CANDLE:
    artist->avt=&candleATV;
    break;
  default:
    ray_error("%s\n", "unknown artist type");
  }

  artist->avt->init(artist);
  if(axes->av->artist_len)
    get_last_artist(axes->av->artist)->next=artist;
  else
    axes->av->artist=artist;

  axes->av->artist_len+=1;
  return artist;
}

static void  artist_line_init(Artist* artist)
{
  Axes* axes=artist->axes;
  ASSERT(artist->data);
  LineData *ldata=artist->data;
  size_t len=artist->view->vd.len;
  Arena* arena=axes->fig->arena;
  assert(ldata->data);
  LineData* pdata=arena_alloc(arena, sizeof(LineData));
  pdata->is_splined=ldata->is_splined;
  size_t tocp=sizeof(*ldata->data)*(ldata->is_splined?len:4);
  pdata->data=arena_alloc(arena,tocp);
  artist->data_len=ldata->is_splined?len:4;
  memcpy(pdata->data,ldata->data,tocp);
  Color* colors=arena_alloc(arena,sizeof(Color));
  if(artist->colors)*colors=*artist->colors;
  else *colors=axes_get_next_tableau_t10_color(&axes->av->tableau_t10_index);
  artist->colors=colors;
  artist->data=pdata;

}


void artist_line_set_data(Artist* artist,  float* data, size_t start, size_t len)
{
  ASSERT(artist && data);
  ASSERT(len>0);
  FigureView *fv=artist->view;
  size_t vd_len=fv->vd.len;
  LineData* ldata=artist->data;
  ASSERTM(vd_len,"invalid state; create at least 1 artist with segmented data\n");
  if(ldata->is_splined)
    {ASSERTM(start+len<=vd_len, "slicing error\n");}
  else
    {ASSERTM(len==4, "non splined lines data len is always 4\n");}
  figure_lock(artist->axes->fig);
  memcpy(ldata->data+start,data, sizeof(*ldata->data)*len);
  figure_unlock(artist->axes->fig);
  artist_invalidate(artist);
}

void artist_candle_set_data(Artist* artist, float* data, size_t start, size_t len)
{
  ASSERT(artist && data && artist->atype == ARTIST_TYPE_CANDLE);
  size_t vd_len=artist->view->vd.len;
  ASSERTM(len && start<=vd_len && len<=vd_len-start, "candle index out of range\n");
  float *dest=artist->data;
  figure_lock(artist->axes->fig);
  for(size_t i=0;i<4;++i)
    memcpy(dest+vd_len*i+start,data+len*i,sizeof(float)*len);
  figure_unlock(artist->axes->fig);
  artist_invalidate(artist);
}

static void  artist_candle_init(Artist* artist)
{
  assert(artist->data);
  Axes* axes=artist->axes;
  size_t len=artist->view->vd.len;
  Arena* arena=axes->fig->arena;
  size_t ta=sizeof(float)*len*4;
  float* cd=arena_alloc(arena,ta);
  artist->data_len=len*4;
  memcpy(cd, artist->data,ta);
  Color* colors=arena_alloc(arena,sizeof(*artist->colors)*2);
  if(artist->colors)memcpy(colors,artist->colors,sizeof(Color)*2);
  else {
    colors[0]=axes_get_next_tableau_t10_color(&axes->av->tableau_t10_index);
    colors[1]=axes_get_next_tableau_t10_color(&axes->av->tableau_t10_index);
  }
  artist->colors=colors;
  artist->data=cd;
}




static Artist* get_last_artist(Artist* artist)
{
  while(artist->next)
    artist=artist->next;
  return artist;
}


static Limit artist_line_get_limit(Artist* artist)
{
  Vd vd=artist->view->vd;
  LineData *l=artist->data;
  if(l->is_splined)  return get_limf(l->data+vd.start,vd.vlen);
  return get_limf(l->data+2,2);
}

static Limit artist_candle_get_limit(Artist* artist){
  Vd vd=artist->view->vd;
  size_t len=artist->view->vd.len;
  float *data=artist->data;
  Limit l1=get_limf(data+(len*1)+vd.start,vd.vlen);//o,[h],l,c
  Limit l2=get_limf(data+(len*2)+vd.start,vd.vlen);//o,h,[l],c
  double array_lim[4]={l1.min,l1.max,l2.min,l2.max};
  Limit lim=get_lim(array_lim,4);
  return lim;
}



static void  artist_line_draw(Artist* artist, Rect area)
{
  Axes* axes=artist->axes;
  Vd vd=artist->view->vd;
  LineData *ldata=artist->data;
  float *data=ldata->data;
  Limit destl=(Limit){.min=area.h+area.y,.max=area.y};
  if(ldata->is_splined){
    size_t rc=1;
    data+=vd.start;
    for(size_t i=1;i<vd.draw_len;++i,++rc){
      DrawLineEx((Vector2){axes->w*(rc-1)+area.x,scale_to_limitf(data[rc-1],axes->av->lim,destl)},
                 (Vector2){axes->w*rc+area.x,scale_to_limitf(data[rc],axes->av->lim,destl)},artist->thick,*artist->colors);
    }
  }
  else{
    double temp[4]={data[0],data[1],data[2],data[3]};
    scale_to_limit(temp+0,temp+0, 2, axes->fig->xlim,(Limit){area.x,area.x+area.w});
    scale_to_limit(temp+2,temp+2, 2, axes->av->lim, destl);
    DrawLineEx((Vector2){temp[0],temp[2]},(Vector2){temp[1],temp[3]},artist->thick,*artist->colors);
  }
}

static void  artist_candle_draw(Artist* artist, Rect area)
{
  Axes* axes=artist->axes;
  Vd vd=artist->view->vd;
  float *data=artist->data;
  float w_use_ratio=0.6;
  float w_use=w_use_ratio*axes->w;
  size_t rc=0;
  for(size_t i=vd.start;i<vd.start+vd.draw_len;++i,++rc){
    double cl[4]={
      data[vd.len*0+i],
      data[vd.len*1+i],
      data[vd.len*2+i],
      data[vd.len*3+i],
    };
    bool is_bear=cl[3]<cl[0];
    Limit limy=(Limit){.min=area.y+area.h,.max=area.y};
    scale_to_limit(cl,cl,4,axes->av->lim,limy);
    float sx=(axes->w*rc)+area.x;
    Color c=artist->colors[is_bear?1:0];
    double min_oc=fmin(cl[0],cl[3]);
    DrawRectangleLines(sx,min_oc,w_use,fabs(cl[0]-cl[3]),c);
    float xmiddle=sx+w_use*0.5;
    DrawLine(xmiddle,cl[1],xmiddle,fmin(cl[0],cl[3]),c);
    DrawLine(xmiddle,fmax(cl[0],cl[3]),xmiddle,cl[2],c);
  }
}


static void artist_line_draw_icon(Artist* artist, Vector2 start_pos)
{
  Theme theme=artist->axes->fig->theme;
  start_pos.y +=theme.font_size/2 ; // draw a line middle as icon
  DrawLineEx(start_pos, (Vector2){start_pos.x + LEGEND_ICON_LEN, start_pos.y}, 1, *artist->colors);
}

static void artist_candle_draw_icon(Artist* artist, Vector2 start_pos)
{  Theme theme=artist->axes->fig->theme;
  int y = theme.font_size / 3;
  DrawRectangleLines(start_pos.x, start_pos.y + y, LEGEND_ICON_LEN,theme.font_size - y * 2,*artist->colors);
}



float* artist_get_data_ptr(Artist* artist)
{
  if (!artist) return NULL;
  if (artist->atype == ARTIST_TYPE_LINE) return ((LineData*)artist->data)->data;
  if (artist->atype == ARTIST_TYPE_CANDLE) return (float*)artist->data;
  return NULL;
}

size_t artist_get_data_len(Artist* artist) { return artist ? artist->data_len : 0; }
void artist_invalidate(Artist* artist) { if (artist) figure_invalidate(artist->axes->fig); }
void artist_set_limitable(Artist* artist, int enabled) { if(artist) { artist->is_limitable=enabled!=0; artist_invalidate(artist); } }
void artist_set_alpha(Artist* artist, unsigned char alpha) { if(artist && artist->colors) { artist->colors[0].a=alpha; if (artist->atype==ARTIST_TYPE_CANDLE) artist->colors[1].a=alpha; artist_invalidate(artist); } }

int artist_candle_update_tick(Artist* artist, double epoch, float price)
{
  if (!artist || artist->atype != ARTIST_TYPE_CANDLE || !artist->view->bs || !isfinite(price)) return -1;
  BarSeries* bs = artist->view->bs;
  figure_lock(artist->axes->fig);
  int shifted = bar_series_update_epoch(bs, epoch);
  if(shifted < 0) { figure_unlock(artist->axes->fig); return -1; }
  size_t n = bs->len, last = n-1;
  float* values = (float*)artist->data;
  if (shifted) {
    for (size_t j=0;j<4;++j) memmove(values+n*j,values+n*j+1,last*sizeof(float));
    values[last]=values[n+last]=values[2*n+last]=values[3*n+last]=price;
  } else {
    if (price > values[n+last]) values[n+last]=price;
    if (price < values[2*n+last]) values[2*n+last]=price;
    values[3*n+last]=price;
  }
  figure_unlock(artist->axes->fig);
  artist_invalidate(artist);
  return shifted;
}

void artist_line_shift(Artist* artist, size_t count)
{
  if(!artist || artist->atype != ARTIST_TYPE_LINE) return;
  LineData* ld=(LineData*)artist->data;
  if (!ld->is_splined || count==0 || count>artist->data_len) return;
  size_t last=artist->data_len-count;
  figure_lock(artist->axes->fig);
  memmove(ld->data,ld->data+count,last*sizeof(float));
  for(size_t i=last;i<artist->data_len;++i) ld->data[i]=NAN;
  figure_unlock(artist->axes->fig);
  artist_invalidate(artist);
}
