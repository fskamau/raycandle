#include "utils.h"
#include "raylib.h"
#include "figure.h"
#include "node.h"
#include "raycandle.h"
#include "artist.h"


#define LEN (size_t)5

double* random_floats(double min, double max, size_t len, void* arena, double smoothness) {
    size_t bytes = len * sizeof(double);
    double* data = (double*)arena_alloc(arena, bytes);
    if (!data) return NULL;

    double range = max - min;
    
    // 1. Start at a random point in the range
    data[0] = min + ((double)rand() / (double)RAND_MAX) * range;

    // 2. Generate subsequent points based on the previous point
    for (size_t i = 1; i < len; i++) {
        // 'smoothness' determines how much the value can change (0.0 to 1.0)
        double jitter = ((double)rand() / (double)RAND_MAX * 2.0 - 1.0) * range * smoothness;
        data[i] = data[i - 1] + jitter;

        // 3. Keep the values within the requested min/max bounds
        if (data[i] > max) data[i] = max;
        if (data[i] < min) data[i] = min;
    }
    
    return data;
}

 
int main(){
  Arena *arena=arena_create(1024*1024*1024);
  NC* nc=nc_create(1024*1024*1024);
  nc_grid(nc,nc->root,2,2);
  Figure* figure=figure_createn(nc);
  figure_set_view_data(figure,LEN,0,(double[]){1,2,0,-1,4,5});
  Theme theme=figure->theme;
  theme.font_path= "/home/ve/Downloads/home/raycandle1/.raycandle/raycandle/fonts/iosevka-regular.ttf";
  figure_set_theme(figure, theme);

  LineData ldata=(LineData){.is_relative=0,
                  .is_splined=1,
                  .data=(double []){30,10,0.0,-30.9,-20,-23,20,400,33}};
  
  artist_create(figure->axes,ARTIST_TYPE_LINE,1.0,NULL,&ldata);

  ldata=(LineData){.is_relative=0,
                  .is_splined=1,
                  .data=(double []){0,1,0.0,-0.9,30,-23,20,400,33}};
  
  artist_create(figure->axes,ARTIST_TYPE_LINE,1.0,NULL,&ldata);

  ldata=(LineData){.is_relative=1,
                  .is_splined=0,
                  .data=(double []){0,1,0.5,0.5}};
  
  artist_create(figure->axes,ARTIST_TYPE_LINE,2.0,&BLACK,&ldata);

  
  ldata=(LineData){.is_relative=0,
                  .is_splined=false,
                  .data=(double []){2,0.3,0.5,0.0,30,20,400,33}};
  
  artist_create(figure->axes,ARTIST_TYPE_LINE,1.0,NULL,&ldata);

  double* data=random_floats(1.02,3.9,LEN*4,arena,3);
  artist_create(figure->axes,ARTIST_TYPE_CANDLE,1.0,NULL,data);

  
    /* artist_create(figure->axes,ARTIST_TYPE_LINE,1.0,&RED,data); */
    /*   artist_create(figure->axes,ARTIST_TYPE_LINE,1.0,&RED,data); */
  figure_show(figure,NULL,720,1960,23);
 
  return 0;
}
