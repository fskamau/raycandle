#include "raycandle.h"
#include "figure.h"
#include "utils.h"
#include <string.h>
#include "artist.h"
#include <math.h>
#include "utils.h"
#include "axes.h"

typedef struct Artist Artist;

static void  artist_candle_init(Artist* artist); //artist->data is double *o,*h,*l,*c
static void  artist_line_init(Artist* artist);
static Artist*   get_last_artist(Artist* artist);
static Limit artist_line_get_limit(Artist* artist);
static Limit artist_candle_get_limit(Artist* artist);

static void  artist_candle_draw(Artist* artist, Rect area);
static void  artist_line_draw(Artist* artist, Rect area);

static void artist_candle_draw_icon(Artist* artist, Vector2 start_pos);
static void artist_line_draw_icon(Artist* artist, Vector2 start_pos);


#define CREATE_AVT(n) static ArtistVTable n##ATV=(ArtistVTable){    \
    .init=artist_##n##_init,                                        \
    .get_limit=artist_##n##_get_limit,                              \
    .draw=artist_##n##_draw,                                        \
    .draw_icon=artist_##n##_draw_icon}


CREATE_AVT(line);
CREATE_AVT(candle);


/*
  creates an artist
  artist->avt should deal with nullable *colors and *data
*/
Artist* artist_create(Axes *axes, ArtistType atype, float thick, Color* colors, const char* label ,void *data)
{
  ASSERT(axes);
  ASSERT(axes->fig->fc->state==FIGURE_STATE_CREATED);
  ASSERTM(axes->fig->fc->vd.len,"len not set; call figure_set_view_data first\n");
  Artist* artist=  arena_alloc(axes->fig->arena,sizeof(Artist));
  memset(artist, 0,sizeof(Artist));
  artist->thick=thick;
  artist->data=data;  
  artist->is_limitable=true;
  artist->colors=colors;
  artist->axes=axes;
  if(label){
      artist->label=str_createf_with_arena(axes->fig->arena, "%s", label);
      size_t x=str_len(artist->label);
      axes->legend.llen=MAX(axes->legend.llen,x);
    }
  switch(atype){
  case ARTIST_TYPE_LINE:
    artist_line_init(artist);
    artist->avt=&lineATV;
    break;
  case ARTIST_TYPE_CANDLE:
    artist_candle_init(artist);
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

static void  artist_line_init(Artist* artist)
{
  Axes* axes=artist->axes;
  ASSERT(artist->data);
  LineData *ldata=artist->data;
  size_t len=axes->fig->fc->vd.len;
  Arena* arena=axes->fig->arena;
  assert(ldata->data);
  if(ldata->is_splined)ASSERT(!ldata->is_relative && "invalid line state\n");
  if(ldata->is_relative){for(size_t i=0;i<4;++i)ASSERTM(ldata->data[i]>=0.f&&ldata->data[i]<=1.f,"range of relative line data is(0.f,1.0f)\n")};
  if(ldata->is_relative)artist->is_limitable=false;
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


void artist_line_set_data(Artist* artist,  double*data, size_t start, size_t len)
{
  ASSERT(artist && data);
  ASSERT(len>0);
  Fconfig *fc=artist->axes->fig->fc;
  size_t vd_len=fc->vd.len;
  LineData* ldata=artist->data;
  ASSERTM(vd_len,"invalid state; create at least 1 artist with segmented data\n");
  if(ldata->is_splined)
    {ASSERTM(start+len<=vd_len, "slicing error\n");}
  else
    {ASSERTM(len==4, "non splined lines data len is always 4\n");}
  memcpy(ldata->data+start,data, sizeof(*ldata->data)*len);
}

void artist_candle_set_data(Artist* artist, double*data, size_t start, size_t len)
{
  ASSERT(artist && data);
  ASSERT(len>0);
  Fconfig *fc=artist->axes->fig->fc;
  size_t vd_len=fc->vd.len;
  double *dest=artist->data;
  ASSERTM(vd_len,"invalid state; create at least 1 artist with segmented data\n");
  ASSERTM(start+len<=vd_len, "slicing index error\n");
  for(size_t i=0;i<4;++i)
    memcpy(dest+vd_len*i+start,data+len*i,sizeof(*dest)*len);
}

static void  artist_candle_init(Artist* artist)
{
  assert(artist->data);
  Axes* axes=artist->axes;
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


static Limit artist_line_get_limit(Artist* artist)
{
  Vd vd=artist->axes->fig->fc->vd;
  LineData *l=artist->data;
  if(l->is_splined)  return get_lim(l->data+vd.start,vd.eff_vlen);
  if(l->is_relative) return (Limit){.min=NAN,.max=NAN};
  return get_lim(l->data+2,2);
}

static Limit artist_candle_get_limit(Artist* artist){
  Axes* axes=artist->axes;
  Vd vd=axes->fig->fc->vd;
  size_t len=axes->fig->fc->vd.len;
  double *data=artist->data;
  Limit l1=get_lim(data+(len*1)+vd.start,vd.eff_vlen);//o,[h],l,c
  Limit l2=get_lim(data+(len*2)+vd.start,vd.eff_vlen);//o,h,[l],c
  double array_lim[4]={l1.min,l1.max,l2.min,l2.max};
  Limit lim=get_lim(array_lim,4);
  return lim;
}



static void  artist_line_draw(Artist* artist, Rect area)
{
  Axes* axes=artist->axes;
  Vd vd=axes->fig->fc->vd;
  LineData *ldata=artist->data;
  double *data=ldata->data;
  Limit destl=(Limit){.min=area.h+area.y,.max=area.y};
  if(ldata->is_splined){
    size_t rc=1;
    data+=vd.start;
    for(size_t i=vd.start+1;i<vd.stop;++i,++rc){
      DrawLineEx((Vector2){axes->w*(rc-1)+area.x,scale_to_limitf(data[rc-1],axes->lim,destl)},
                 (Vector2){axes->w*rc+area.x,scale_to_limitf(data[rc],axes->lim,destl)},artist->thick,*artist->colors);
    }    
  }
  else{   
    double x[2]={0};
    double y[2]={0};
    Limit ylim=(ldata->is_relative)?(Limit){0.f,1.f}:axes->lim;
    Limit xlim=(ldata->is_relative)?(Limit){0.f,1.f}:axes->fig->xlim;
    scale_to_limit(data,x, 2, xlim,(Limit){area.x,area.x+area.w});
    scale_to_limit(data+2, y, 2, ylim, destl);
    DrawLineEx((Vector2){*x,*y},(Vector2){x[1],y[1]},artist->thick,*artist->colors);
  }
}

static void  artist_candle_draw(Artist* artist, Rect area)
{
  Axes* axes=artist->axes;
  Vd vd=axes->fig->fc->vd;
  double *data=artist->data;
  float w_use_ratio=0.6;
  float w_use=w_use_ratio*axes->w;
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
    float sx=(axes->w*rc)+area.x;
    Color c=artist->colors[(cl[0]>cl[3])?0:1];
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
