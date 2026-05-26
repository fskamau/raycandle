#include "raycandle.h"
/* #include "raylib.h" */
#include "figure.h"
#include "utils.h"
#include <string.h>
#include "artist.h"
#include <math.h>
#include "utils.h"

typedef struct Artist Artist;
typedef enum ArtistType ArtistType;
static void  artist_candle_init(Artist* artist,Axes* axes); //artist->data is double *o,*h,*l,*c
static void  artist_line_init(Artist* artist,Axes* axes);
static Artist*   get_last_artist(Artist* artist);
static Limit artist_line_get_limit(Artist* artist, Axes* axes);
static Limit artist_candle_get_limit(Artist* artist, Axes* axes);

static void  artist_candle_draw(Artist* artist,Axes* axes,Rect area);
static void  artist_line_draw(Artist* artist,Axes* axes,Rect area);


#define CREATE_AVT(n) static ArtistVTable n##ATV=(ArtistVTable){    \
    .init=artist_##n##_init,                                        \
    .get_limit=artist_##n##_get_limit,                              \
    .draw=artist_##n##_draw}

CREATE_AVT(line);
CREATE_AVT(candle);


/*
  creates an artist
  artist->avt should deal with nullable *colors and *data
*/
Artist* artist_create(Axes *axes, ArtistType atype, float thick,Color* colors, void *data){
  ASSERT(axes);
  ASSERTM(axes->fig->fc->vd.len,"len not set; call figure_set_view_data first\n");
  Artist* artist=  arena_alloc(axes->fig->arena,sizeof(Artist));
  memset(artist, 0,sizeof(Artist));
  artist->thick=thick;
  artist->colors=colors;
  artist->data=data;
  artist->is_limitable=true;
  switch(atype){
  case ARTIST_TYPE_LINE:
    artist_line_init(artist, axes);
    artist->avt=&lineATV;
    break;
  case ARTIST_TYPE_CANDLE:
    artist_candle_init(artist,axes);
    artist->avt=&candleATV;
    break;
  default:
    ray_error("%s\n", "unknown artist type");
  }
  if(axes->artist_len)
    get_last_artist(axes->artist)->next=artist;
  else
    axes->artist=artist;
  axes->artist_len+=1;
  return artist;
}

static void  artist_line_init(Artist* artist, Axes* axes)
{
  ASSERT(artist->data);
  LineData *ldata=artist->data;
  size_t len=axes->fig->fc->vd.len;
  Arena* arena=axes->fig->arena;
  assert(ldata->data);
  if(ldata->is_splined)ASSERT(!ldata->is_relative && "invalid line state\n");
  if(ldata->is_relative){for(size_t i=0;i<4;++i)ASSERTM(ldata->data[i]>=0.f&&ldata->data[i]<=1.f,"range of relative line data is(0.f,1.0f)\n")};
  LineData* pdata=arena_alloc(arena, sizeof(LineData));
  pdata->is_relative=ldata->is_relative;
  pdata->is_splined=ldata->is_splined;
  size_t tocp=sizeof(*ldata->data)*(ldata->is_relative?4:len);
  pdata->data=arena_alloc(arena,tocp);
  memcpy(pdata->data,ldata->data,tocp);
  Color* colors=arena_alloc(arena,sizeof(Color));
  if(artist->colors)*colors=*artist->colors;
  else *colors=axes_get_next_tableau_t10_color(axes);
  artist->colors=colors;
  artist->data=pdata;
}


static void  artist_candle_init(Artist* artist, Axes* axes)
{
  assert(artist->data);
  size_t len=axes->fig->fc->vd.len;
  Arena* arena=axes->fig->arena;
  size_t ta=sizeof(double)*len*4;
  double* cd=arena_alloc(arena,ta);
  memcpy(cd, artist->data,ta);
  Color* colors=arena_alloc(arena,sizeof(*artist->colors)*2);
  if(artist->colors)memcpy(colors,artist->colors,sizeof(Color)*2);
  else {
    colors[0]=axes_get_next_tableau_t10_color(axes);
    colors[1]=axes_get_next_tableau_t10_color(axes);
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


static Limit artist_line_get_limit(Artist* artist, Axes* axes)
{
  (void)axes;
  Vd vd=axes->fig->fc->vd;
  LineData *l=artist->data;
  if(l->is_splined)  return get_lim(l->data+vd.start,vd.stop-vd.start);
  if(l->is_relative)return (Limit){.min=NAN,.max=NAN};
  else return get_lim(l->data+2,2);
}

static Limit artist_candle_get_limit(Artist* artist, Axes* axes){
  Vd vd=axes->fig->fc->vd;
  size_t len=axes->fig->fc->vd.len;
  double *data=artist->data;
  Limit l1=get_lim(data+(len*1)+vd.start,vd.stop-vd.start);//o,[h],l,c
  Limit l2=get_lim(data+(len*2)+vd.start,vd.stop-vd.start);//o,h,[l],c
  double array_lim[4]={l1.min,l1.max,l2.min,l2.max};
  Limit lim=get_lim(array_lim,4);
  return lim;
}



static void  artist_line_draw(Artist* artist, Axes* axes, Rect area)
{
  Vd vd=axes->fig->fc->vd;
  LineData *ldata=artist->data;
  double *data=ldata->data;
  float xmiddle=0.5*axes->w;
  Limit destl=(Limit){.max=area.y,.min=area.h+area.y};
  size_t rc=1;
  if(ldata->is_splined){
    for(size_t i=vd.start+1;i<vd.stop;++i,rc++){
      DrawLineEx((Vector2){axes->w*(rc-1)+area.x+xmiddle,scale_to_limitf(data[i-1],axes->lim,destl)},
                 (Vector2){axes->w*rc+area.x+xmiddle,scale_to_limitf(data[i],axes->lim,destl)},artist->thick,*artist->colors);
    }
  }else{
    double x[2]={0};
    double y[2]={0};
    Limit ylim=(ldata->is_relative)?(Limit){0.f,1.f}:axes->lim;
    Limit xlim=(ldata->is_relative)?(Limit){0.f,1.f}:axes->fig->xlim;
    scale_to_limit(data,x, 2, xlim, (Limit){area.x,area.x+area.w});
    scale_to_limit(data+2, y, 2, ylim, destl);
    DrawLineEx((Vector2){*x,*y},(Vector2){x[1],y[1]},artist->thick,*artist->colors);
  }
}

static void  artist_candle_draw(Artist* artist, Axes* axes, Rect area)
{
  Vd vd=axes->fig->fc->vd;
  double *data=artist->data;
  float w_use_ratio=0.6;
  float w_use=w_use_ratio*axes->w;
  float w_offset=(1.f-w_use_ratio)*0.0*axes->w;
  size_t rc=0;
  for(size_t i=vd.start;i<vd.stop;++i,++rc){
    double cl[4]={
      data[vd.len*0+i],
      data[vd.len*1+i],
      data[vd.len*2+i],
      data[vd.len*3+i],
    };
    Limit limy=(Limit){.min=area.y+area.h,.max=area.y};
    scale_to_limit(cl,cl,4,axes->lim,limy);
    float sx=(axes->w*rc)+w_offset+area.x;
    Color c=artist->colors[(cl[0]>cl[3])?0:1];
    double min_oc=fmin(cl[0],cl[3]);
    DrawRectangleLines(sx,min_oc,w_use,fabs(cl[0]-cl[3]),c);
    float xmiddle=sx+w_use*0.5;
    DrawLine(xmiddle,cl[1],xmiddle,fmin(cl[0],cl[3]),c);
    DrawLine(xmiddle,fmax(cl[0],cl[3]),xmiddle,cl[2],c);
  }
}

