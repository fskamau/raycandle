#include "utils.h"
#include "raylib.h"
#include "figure.h"
#include "node.h"
#include "raycandle.h"
#include "artist.h"
#include <string.h>

#define LEN (size_t)3000
int main(){
  Arena *arena=arena_create(1024*1024*1024);
  NC* nc=nc_create(1024*1024*1024);
  nc_grid(nc,nc->root,2,1);
  Figure* figure=figure_createn(nc);
  /* figure_create(1,1); */
  
  double* data=arena_alloc(arena,sizeof(double)*LEN*(4+1));
  FILE* f=fopen("data.bin", "rb");
  ASSERT(f);
  fread(data, sizeof(double),LEN*(4+1),f);
  fclose(f);

  Theme theme=figure->theme;
  /* theme.font_size=32; */
  theme.font_path= "assets/font.ttf";
  figure_set_theme(figure, theme);
  

  LineData ldata;

  double *mym=arena_alloc(arena,sizeof(double)*LEN);
  memcpy(mym, data+LEN,sizeof(double)*LEN);
  ldata=(LineData){.is_relative=0,
                  .is_splined=1,
                  .data=mym};


  figure_set_view_data(figure,LEN,"%a/%d/%m %H:%M:%S",data,60);

  /* artist_create(figure->axes+1,ARTIST_TYPE_LINE,1.0,&RED,"more",&ldata); */

  
  /* artist_create(figure->axes,ARTIST_TYPE_LINE,2.0,NULL,&ldata); */
    
  artist_create(figure->axes,ARTIST_TYPE_CANDLE,1.0,(Color[2]){GetColor(0x079981ff),GetColor(0xf23645ff)},NULL,data+LEN);

    /* x->is_limitable=0; */
  
  
  /* artist_create(figure->axes+2,ARTIST_TYPE_CANDLE,1.0,(Color[2]){GetColor(0x079981ff),GetColor(0xf23645ff)},"dmkd",data+LEN); */
  /*     artist_create(figure->axes+2,ARTIST_TYPE_CANDLE,1.0,(Color[2]){GetColor(0x079981ff),GetColor(0xf23645ff)},NULL,data+LEN); */
  /*   artist_create(figure->axes+2,ARTIST_TYPE_CANDLE,1.0,(Color[2]){GetColor(0x079981ff),GetColor(0xf23645ff)},"mdkmfkdldmdlkdmkd",data+LEN); */

    figure_set_title(figure,"mksd:");  
    /* artist_create(figure->axes,ARTIST_TYPE_LINE,1.0,&RED,data); */
    /*   artist_create(figure->axes,ARTIST_TYPE_LINE,1.0,&RED,data); */
    /* for(size_t i=0;i<6;++i) */
    /*   axes_set_yformatter(figure->axes+i, "%.2f"); */
  figure_show_cursors(figure);
  figure_show_legends(figure, LEGEND_POSITION_BOTTOM_LEFT);
  figure_show(figure,"%f",1280,720,50);
 
  return 0;
}
