#include "utils.h"
#include "raylib.h"
#include "figure.h"
#include "node.h"
#include "raycandle.h"
#include "artist.h"


#define LEN (size_t)100
int main(){
  Arena *arena=arena_create(1024*1024*1024);
  NC* nc=nc_create(1024*1024*1024);
  nc_grid(nc,nc->root,3,2);
  Figure* figure=figure_createn(nc);

  
  double* data=arena_alloc(arena,sizeof(double)*LEN*(4+1));
  FILE* f=fopen("data.bin", "rb");
  ASSERT(f);
  fread(data, sizeof(double),LEN*(4+1),f);
  fclose(f);

  figure_set_view_data(figure,LEN,0,data);
  Theme theme=figure->theme;
  /* theme.font_size=32; */
  theme.font_path= "/home/ve/Downloads/home/raycandle1/.raycandle/raycandle/fonts/iosevka-regular.ttf";
  figure_set_theme(figure, theme);
  

  LineData ldata;
    
  ldata=(LineData){.is_relative=0,
                  .is_splined=1,
                  .data=data+(LEN*4)};
  
  /* artist_create(figure->axes,ARTIST_TYPE_LINE,2.0,NULL,&ldata); */
    
  artist_create(figure->axes,ARTIST_TYPE_CANDLE,1.0,(Color[2]){GetColor(0x079981ff),GetColor(0xf23645ff)},data+LEN);

  
    /* artist_create(figure->axes,ARTIST_TYPE_LINE,1.0,&RED,data); */
    /*   artist_create(figure->axes,ARTIST_TYPE_LINE,1.0,&RED,data); */
  figure_show(figure,NULL,720,1280,20);
 
  return 0;
}
