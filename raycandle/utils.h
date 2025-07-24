#include "raycandle.h"
#include <stdio.h>
#include <stdlib.h>
#include "cust_malloc.h"
#include <math.h>

#define RC_ONCE_AFTER_INIT_WINDOW // identifies a function is called once after InitWindow
#define RC_DATA_IN_LIMIT(data, limit) (((limit).limit_min <= ((data))) && (data) <= ((limit).limit_max))

// NOTE:this 4 macros do not check whether data is in within limit. use RC_DATA_IN_LIMIT
#define RC_PIXEL_X_2_DATA(pixel, axes) (((((pixel) - (axes)->startX) / (double)(axes)->width) * ((axes)->parent->dragger.xlimit.diff)) + (axes)->parent->dragger.xlimit.limit_min)
#define RC_PIXEL_Y_2_DATA(pixel, axes) (((((axes)->height - ((pixel) - (axes)->startY)) /(double) (axes)->height) * (axes)->ylocator.limit.diff) + (axes)->ylocator.limit.limit_min)
#define RC_DATA_X_2_PIXEL(data, axes) (((((data) - (axes)->parent->dragger.xlimit.limit_min) / (axes)->parent->dragger.xlimit.diff) * (axes)->width) + (axes)->startX)
#define RC_DATA_Y_2_PIXEL(data, axes) (((((axes)->ylocator.limit.limit_max - (data)) / (axes)->ylocator.limit.diff) * (axes)->height) + (axes)->startY)
#define RC_COLOR1_EQUALS_COLOR2(color1, color2) ((color1).r == (color2).r && (color1).g == (color2).g && (color1).b == (color2).b && (color1).a == (color2).a)

#define RC_MIN(val, min) ((val) < (min) ? (val) : (min))
#define RC_MAX(val, max) ((val) > (max) ? (val) : (max))

#define RC_ECHO(__any) #__any
#define FIGURE_FONT(__pfigure) (*(Font *)((__pfigure)->font))

#if RAYCANDLE_DEBUG
#include <assert.h>
#define RC_ASSERT assert
#else 
#define RC_ASSERT(cond) do {if(!(cond)){RC_ERROR("condition '%s' failed\n",#cond);}}while(0)
#endif


typedef enum {
  LOG_LEVEL_INFO = 0,
  LOG_LEVEL_WARN = 1,
  LOG_LEVEL_ERROR = 2,
} Log_Level;
void set_log_level(Log_Level Log_Level);

#define RC_INFO(error_format, ...) RC_write_std(LOG_LEVEL_INFO, __FILE__,__LINE__,__func__, error_format,##__VA_ARGS__)
#define RC_WARN(error_format, ...) RC_write_std(LOG_LEVEL_WARN, __FILE__,__LINE__,__func__, error_format,##__VA_ARGS__)
#define RC_ERROR(error_format, ...)RC_write_std(LOG_LEVEL_ERROR, __FILE__,__LINE__,__func__, error_format,##__VA_ARGS__)

#define RAYCANDLE_MAX_ERROR_STR 1024
#define RC_EXIT RC_ERROR("JUST EXITED HERE\n");
void RC_write_std(Log_Level log_level,const char file[],int line,const char func[],const char* error_format,...);

static const Color tableau_colors[10] = {
    {31, 119, 191, 255},  // Blue
    {255, 127, 14, 255},  // Orange
    {44, 160, 44, 255},   // Green
    {214, 39, 40, 255},   // Red
    {148, 103, 189, 255}, // Purple
    {140, 86, 75, 255},   // Brown
    {227, 119, 194, 255}, // Pink
    {127, 127, 127, 255}, // Gray
    {188, 189, 34, 255},  // Olive
    {23, 190, 207, 255}   // Cyan
};

typedef enum {
  RC_ALIGNMENT_LEFT,
  RC_ALIGNMENT_CENTER,
  RC_ALIGNMENT_RIGHT,
} Rc_Alignment;

float align_text(Font *font, const char *text, int width, int font_size, int spacing,Rc_Alignment alignment); // returns startx for the text

Color RC_faded_color_from(Color original);
Color axes_get_next_tableau_t10_color(Axes *axes);

/*
if need be to add more logic since show(figure) will block, these 3 functions may be used:
InitWindow();
while (!WindowShouldClose()){

}
CloseWindow();

translates to :

raylib_init()// calls InitWindow()
while(!WindowShouldClose()){
  ClearBackground();
  raylib_init_loop()//set up controls and ajust figure size
  BeginDrawing()
  //any more logic here
  update_figure() // will draw artists
  EndDrawing()
}
CloseWindow()

*/
void raylib_init(Figure *figure);   // init window
void raylib_init_loop();// routines for raylib  in each loop
bool update_figure(Figure *figure); // main update


/*some figure defaults*/
#define FIGURE_DEFAULT() create_figure((int[]){960,720},NULL,WHITE,1,1,1,(size_t[]){0,0,1,1},(char[]){'a'},0.01,20,20,0);

#undef CUSM_PREFIX
