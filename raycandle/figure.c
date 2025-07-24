#include "figure.h"
#include "axes.h"
#include "locator.h"
#include "mouse_updater.h"
#include "raycandle.h"
#include <locale.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

static void figure_draw_cursors(Figure *figure);                // draw cursor positions
static bool set_real_span_skel_map(Figure *figure);             // map axes skeletons to real pixels
static bool set_borders(Figure *figure);                        // returns if borders can be set
RC_ONCE_AFTER_INIT_WINDOW static void set_font(Figure *figure); // load font
static void update_fps(Figure *figure);                         // set fps according to figure->fps
static void draw_title(Figure *figure);                         // draw title
static void draw_tooltip(Figure *figure);                       // draw tooltip data
static void draw_current_time(Figure* figure);  //show current time. might be used if the app is doing nothing

static void figure_draw_cursors(Figure *figure) {
  size_t axes_under_mouse;
  if ((axes_under_mouse = get_axes_index_under_mouse(figure)) == figure->axes_len) {
    return;
  }
  double xdata, ydata, zdata;
  Vector2 v = GetMousePosition();
  Axes axes = figure->axes[axes_under_mouse];
  if (axes.artist_len == 0) {return;} // all axes share same x-axis
  xdata = RC_PIXEL_X_2_DATA(v.x, &axes);
  ydata = RC_PIXEL_Y_2_DATA(v.y, &axes);
  for (size_t i = 0; i < figure->axes_len; ++i) {
    axes = figure->axes[i];
    if (axes.artist_len == 0) {continue;}
    if (RC_DATA_IN_LIMIT(xdata, axes.parent->dragger.xlimit)) {
      zdata = RC_DATA_X_2_PIXEL(xdata, &axes);
      for (int i = axes.startY; i < (int)(axes.startY + axes.height); ++i) {DrawPixel(zdata, i, figure->text_color);}
    }
    if (RC_DATA_IN_LIMIT(ydata, axes.ylocator.limit)) {
      zdata = RC_DATA_Y_2_PIXEL(ydata, &axes);
      for (int i = axes.startX; i < (int)(axes.startX + axes.width); ++i) {
        DrawPixel(i, zdata, figure->text_color);
      }
    }
  }
}

static bool set_real_span_skel_map(Figure *figure) {
  switch (figure->sds) {
  case SCREEN_DIMENSION_STATE_DEFAULT:
    RC_ERROR("condition '%s' is unreachable after InitWindow!\n", RC_ECHO(SCREEN_DIMENSION_STATE_DEFAULT));
    return false;//not reachable
  case SCREEN_DIMENSION_STATE_UNCHANGED: {
    for (size_t axes_index = 0; axes_index < figure->axes_len; axes_index++) {
      size_t index = axes_index * 4;
      figure->axes[axes_index].startX = figure->axes_skels_copy[index];
      figure->axes[axes_index].startY = figure->axes_skels_copy[index + 1];
      figure->axes[axes_index].width = figure->axes_skels_copy[index + 2];
      figure->axes[axes_index].height = figure->axes_skels_copy[index + 3];
    }
    break;
  }
  case SCREEN_DIMENSION_STATE_CHANGED: {
    size_t axes_skels_copy[4] = {0};
    long int x= (long int)figure->height-(figure->font_size*(figure->title == NULL ? figure->show_tooltip?1:0 : 2)); // may underflow if height<font_size
    if(x<0){return false;}
    size_t axes_sizes[] = {figure->width / figure->cols,  x / figure->rows};
    for (size_t axes_index = 0; axes_index < figure->axes_len; axes_index++) {
      size_t index = axes_index * 4;
      // set new absolute measurements
      axes_skels_copy[0] = axes_sizes[0] * figure->axes_skels[index];
      axes_skels_copy[1] = axes_sizes[1] * figure->axes_skels[index + 1] + (figure->title == NULL ? 0 :figure->show_tooltip? figure->font_size:0);
      axes_skels_copy[2] = axes_sizes[0] * figure->axes_skels[index + 2];
      axes_skels_copy[3] = axes_sizes[1] * figure->axes_skels[index + 3];
      // ajdust the measurents  by removing border, creating space at the top for title height if title!=null   and
      // creating space at the bottom for tooltip
      size_t b0 = figure->axes_skels[index] == 0 ? figure->border_dimensions[0] : (size_t)figure->border_dimensions[0] / 2;
      axes_skels_copy[0] += b0;
      axes_skels_copy[2] -= (size_t)(figure->axes_skels[index + 2] + figure->axes_skels[index] == figure->cols ? figure->border_dimensions[0] : figure->border_dimensions[0] / 2) + b0;
      size_t b1 = (size_t)(figure->border_dimensions[1] + figure->border_dimensions[1] / 2);
      axes_skels_copy[1] += b1;
      axes_skels_copy[3] -= (size_t)(figure->axes_skels[index + 1] + figure->axes_skels[index + 3] == figure->rows ? figure->border_dimensions[1] : figure->border_dimensions[1] / 2) + b1;
      // then copy these measurements into the axes
      figure->axes[axes_index].startX = figure->axes_skels_copy[index] = axes_skels_copy[0];
      figure->axes[axes_index].startY = figure->axes_skels_copy[index + 1] = axes_skels_copy[1];
      figure->axes[axes_index].width = figure->axes_skels_copy[index + 2] = axes_skels_copy[2];
      figure->axes[axes_index].height = figure->axes_skels_copy[index + 3] = axes_skels_copy[3];
    }
  } break;
  }
  return true;
}

static bool set_borders(Figure *figure) {
  figure->border_dimensions[0] = (size_t)(figure->border_percentage * figure->width) / (figure->cols + 1);
  long int x=(long int)figure->height-(figure->font_size * (figure->title == NULL ? figure->show_tooltip?1:0 : 2));
  if(x<0){return false;}
  figure->border_dimensions[1] = (size_t)(figure->border_percentage * x) / (figure->rows + 1);
  return true;
}

RC_ONCE_AFTER_INIT_WINDOW static void set_font(Figure *figure) {
  figure->font = cm_malloc(sizeof(Font), RC_ECHO(Font));
  Font font = LoadFontEx("fonts/font.ttf", figure->font_size, 0, 250);
  if (font.baseSize != figure->font_size) {
    font = GetFontDefault();
    RC_WARN("could not load './fonts/font.ttf'; using default font\n");
  }
  *((Font *)figure->font) = font;
}

void raylib_init(Figure *figure){
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  SetTraceLogLevel(LOG_ERROR);
  char* window_title=figure->window_title?figure->window_title:string_create_from_format(0,NULL,"%s.%d",RAY_WINDOW_TITLE,getpid());
  InitWindow(figure->width, figure->height,window_title);
  string_destroy(window_title);
  set_font(figure);
  update_fps(figure);
}

static void update_fps(Figure *figure) {
  if (figure->fps < 0) {
    RC_ERROR("fps must be 0 or non negative value\n");
  }
  SetTargetFPS(figure->fps);
}

void raylib_init_loop() {
  // keyboard
  //  F -> Maximize
  if (IsKeyPressed(KEY_F)) {
    if (IsWindowMaximized()) {
      RestoreWindow();
    } else {
      MaximizeWindow();
    }
  }
  // Q -> exit
  SetExitKey(KEY_Q);
}
    
static void draw_current_time(Figure* figure){
#define BUFFER_LEN 40
  struct timeval tv;
  struct tm* tm_info;
  char _buffer[BUFFER_LEN]={0};
  Str buffer=string_create(BUFFER_LEN,_buffer);
  gettimeofday(&tv, NULL);
  tm_info = localtime(&tv.tv_sec);
  string_append(buffer, "%04d-%02d-%02d   %02d:%02d:%02d   %02ld",
		tm_info->tm_year + 1900,
		tm_info->tm_mon + 1,
		tm_info->tm_mday,
		tm_info->tm_hour,
		tm_info->tm_min,
		tm_info->tm_sec,
		tv.tv_usec/10000);
  float x = align_text(figure->font, buffer, figure->width-10, figure->font_size, figure->font_spacing, RC_ALIGNMENT_CENTER);
  DrawTextEx(*(Font *)figure->font,buffer, (Vector2){x, figure->height/2}, figure->font_size, figure->font_spacing, figure->text_color);
}

bool update_figure(Figure *figure) {
  int sd[] = {GetScreenWidth(), GetScreenHeight()};
  figure->sds = (sd[0] ==figure->width &&sd[1] ==figure->height) ? SCREEN_DIMENSION_STATE_UNCHANGED: SCREEN_DIMENSION_STATE_CHANGED; // cannot be SCREEN_DIMENSION_STATE_DEFAULT
  if(figure->force_update){
    figure->force_update=false;
    figure->sds=SCREEN_DIMENSION_STATE_CHANGED;
    if(figure->has_dragger){update_from_position(figure->dragger.cur_position, figure);}
  }
  figure->width = sd[0];
  figure->height = sd[1];
  if(!set_borders(figure)){return false;}
  if(!set_real_span_skel_map(figure)){return false;}
  if(figure->title!=NULL){draw_title(figure);}
  if(IsKeyPressed(KEY_LEFT_SHIFT)||IsKeyPressed(KEY_RIGHT_SHIFT)){figure->clear_screen=!figure->clear_screen;}
  if(figure->clear_screen){
    figure->force_update=true;
    draw_current_time(figure);
    return true;
  }
  for (size_t i = 0; i < figure->axes_len; ++i) {if(!draw_axes(figure->axes + i)){return false;}}
  if (figure->dragger.update_len > 0) {
    mouse_updates(figure);
  }
  if (figure->show_cursors == true) {
    figure_draw_cursors(figure);
  }
  if(figure->show_tooltip){draw_tooltip(figure);}
  return true;
}

static void draw_title(Figure *figure) {
  RC_ASSERT(figure->title!=NULL);
  float x = align_text(figure->font, figure->title, figure->width, figure->font_size, figure->font_spacing, RC_ALIGNMENT_CENTER);
  DrawTextEx(*(Font *)figure->font, figure->title, (Vector2){x, 0}, figure->font_size, figure->font_spacing, figure->text_color);
}

static void draw_tooltip(Figure *figure) {
  /*
    fps  [index_under_mouse/total_length] [current_time-xindex[-1]] timeframe 'axes_label_under_mouse' x_index_under_mouse y_index_under_mouse
  */
  RC_ASSERT(figure->show_tooltip);
  #define buf_size  128
  char _buffer[buf_size];
  char* buffer=string_create(buf_size,_buffer);  
  size_t axes_under_mouse = get_axes_index_under_mouse(figure);
  Axes *axes = figure->axes + axes_under_mouse;
  string_append(buffer,"fps=%'d  mem:%'zu ", GetFPS(),cm_malloc_size());
  if (figure->has_dragger) { // the user has called `set_dragger` because otherwise we do not have xdata
    if (figure->axes_len != axes_under_mouse && axes->artist_len != 0) {
      int mousex_iloc=(int)(((float)(GetMouseX()-axes->startX)/(axes->width)*figure->dragger.visible_data)+figure->dragger.start);
      string_append(buffer," [%d/%zu] ",mousex_iloc,(figure->dragger.len_data-1));
      string_append(buffer,"timeframe=%zu ", figure->dragger.timeframe);
      locator_tooltip_mouse_position(axes, buffer, GetMouseX(), GetMouseY());
    } else
    {
      string_append(buffer,"Xindex[-1]=");
      formatter_to_str(figure->dragger.xformatter,figure->dragger.xlim_format,buffer,&figure->dragger.xdata[figure->dragger.len_data-1]);
    }
  }
  float x = figure->border_dimensions[0]+AXES_FRAME_THICK+align_text(figure->font, buffer, (size_t)figure->width - (figure->border_dimensions[0]+AXES_FRAME_THICK), figure->font_size, figure->font_spacing, RC_ALIGNMENT_LEFT);
  DrawTextEx(*(Font *)figure->font, buffer, (Vector2){x, figure->height - figure->font_size}, figure->font_size, figure->font_spacing, figure->text_color);
}

void update_xlim_shared(Figure *figure) {
  size_t y = figure->dragger.start;
  size_t cp=figure->dragger.cur_position;
  bool at_end=cp==figure->dragger.len_data;
  double lmax,lmin,diff;
  lmax=figure->dragger.xdata[RC_MIN(cp,figure->dragger.len_data-1)]+(at_end?figure->dragger.timeframe:0);
  lmin=figure->dragger.xdata[cp-figure->dragger.visible_data];
  diff=lmax-lmin;
  figure->dragger.xlimit=(Limit){.limit_max=lmax,.limit_min=lmin,.diff=lmax-lmin,.is_static=figure->dragger.xlimit.is_static};
  for (size_t i = 0; i < figure->dragger.visible_data; ++i, y++) {
    double x = (size_t)figure->dragger.xdata[y] % (size_t)figure->dragger.timeframe; // donot treat minimoves like real spacings
    figure->dragger.xdata_shared[i] = RC_DATA_IN_LIMIT(figure->dragger.xdata[y], figure->dragger.xlimit) ? (figure->dragger.xdata[y] - (x + lmin)) / diff : NAN;
  }
}

Figure *create_figure(int *fig_size, char *window_title, Rc_Color background_color, size_t axes_len, size_t rows, size_t cols, size_t *axes_skel, char *labels, float border_percentage, int fps, size_t font_size, int font_spacing){
  if (border_percentage < 0.f || border_percentage >= 1.f) {RC_WARN("border_percentage is %f\n", border_percentage);}
  if (font_size == 0) {RC_ERROR("font_size is 0\n");}
  setlocale(LC_NUMERIC, "");
  size_t *border_dimensions = cm_malloc(sizeof(size_t) * 2, RC_ECHO(border_dimensions));
  size_t *axes_skels_copy = cm_malloc(sizeof(size_t) * axes_len * 4, RC_ECHO(axes_skels_copy));
  size_t *axes_skels_dyn = cm_malloc(sizeof(size_t) * axes_len * 4, RC_ECHO(axes_skels_dyn));
  memcpy(axes_skels_dyn, axes_skel, sizeof(axes_skel[0]) * axes_len * 4);
  Figure *figure = cm_malloc(sizeof(Figure), RC_ECHO(Figure));
  *figure = (Figure){
      .width = fig_size[0],
      .height = fig_size[1],
      .fps = fps,
      .font_size = font_size,
      .font_spacing = font_spacing,
      .border_percentage = border_percentage,
      // .dragger=set by calling set_dragger
      .title =NULL,
      .window_title=window_title?string_create_from_format(0,NULL,"%s",window_title):window_title,
      .axes_len = axes_len,
      .rows = rows,
      .cols = cols,
      .axes_skels = axes_skels_dyn,
      .axes_skels_copy = axes_skels_copy,
      .axes = NULL,
      .border_dimensions = border_dimensions,
      .font = NULL,
      .mouseinfo = (MouseInfo){.down = false, .wait_up = false},
      .axes_frame_color = BLACK,
      .background_color = background_color,
      .text_color = BLACK,
      .sds = SCREEN_DIMENSION_STATE_DEFAULT,
      .show_cursors = false,
      .force_update=true,
      .has_dragger=false,
      .show_tooltip=true,
      .clear_screen=false,

  };
  create_axes(figure, labels);
  return figure;
}

void show(Figure* figure){
  RC_ASSERT(figure!=NULL);
  RC_ASSERT(figure->sds == SCREEN_DIMENSION_STATE_DEFAULT);
  figure->sds = SCREEN_DIMENSION_STATE_CHANGED;
  if(figure->has_dragger){
    update_from_position(figure->dragger.cur_position,figure);
  }
  raylib_init(figure);
  axes_set_legend(figure);
  //trigger updates in the first loop
  figure->force_update=true;
  while (!WindowShouldClose()) {
    raylib_init_loop();
    BeginDrawing();
    ClearBackground(figure->background_color);
    if(!update_figure(figure)&&(RAYCANDLE_DEBUG)){RC_INFO("figure size is too small, some data will not be visible\n");}
    EndDrawing();
  }
  CloseWindow();
}

void figure_set_title(Figure *figure, char *title) {
  RC_ASSERT(title!=NULL);
  if (figure->title != NULL){string_destroy(figure->title);}
  else{
    //force update
    figure->force_update=true;
  }
  figure->title = string_create_from_format(0,NULL,"%s",title);
}

#define XLIM_FORMAT_LEN 128
void set_dragger(Figure *figure, Dragger dragger) {
  if (figure->has_dragger||figure->dragger.len_data != 0 || figure->sds != SCREEN_DIMENSION_STATE_DEFAULT){RC_ERROR("'%s' may only be called once before InitWindow\n", RC_ECHO(set_dragger));}
  if (dragger.len_data == 0) {RC_ERROR("Dragger len_data is 0, no xdata?\n");}
  if (dragger.visible_data == 0) {dragger.visible_data = dragger.len_data;}
  if (dragger.visible_data > dragger.len_data) {RC_ERROR("dragger.visible_data>dragger.len_data\n");}
  if (dragger.update_len > dragger.visible_data) {RC_ERROR("update length(%zu)  must be <= visible data (%zu)\n", dragger.update_len, dragger.visible_data);}
  // set spacing. it must not be a nan or 0.
  if (dragger.timeframe < 0.f) {RC_ERROR("spacing must not be NaN or zero\n");}
  dragger.cur_position = dragger.visible_data;
  dragger.start = dragger.cur_position - dragger.visible_data;
  if (dragger.xlim_format == NULL && dragger.xformatter != FORMATTER_NULL_FORMATTER) {
    RC_ERROR("'%s' is NULL but formatter '%s' is not '%s'\n", RC_ECHO(dragger.xlim_format), RC_ECHO(dragger.formatter), RC_ECHO(FORMATTER_NULL_FORMATTER));
  }
  Str buffer = string_create(XLIM_FORMAT_LEN,NULL);
  string_append(buffer,"%s",dragger.xlim_format);
  dragger.xlim_format=buffer;
  dragger.xdata_shared = cm_malloc(dragger.visible_data * sizeof(double), RC_ECHO(dragger.xdata_shared));
  for (size_t i = 0; i < figure->axes_len; ++i) {
    figure->axes[i].xdata_buffer = cm_malloc(dragger.visible_data * sizeof(double), RC_ECHO(figure->axes.xdata_buffer));
  }
  figure->dragger = dragger;
  figure->has_dragger=true;
}
#undef XLIM_FORMAT_LEN

void update_timeframe(Figure* figure,size_t timeframe){
  if(figure->has_dragger==false){RC_ERROR("a dragger need to be set first; call `set_dragger`\n");}
  if (timeframe < 0.f) {RC_ERROR("timeframe must not be NaN or zero\n");}
  figure->dragger.timeframe=timeframe;
}


