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
static void update_fps(Figure *figure);                         // set fps according to figure->fps
static void draw_title(Figure *figure);                         // draw title
static void draw_tooltip(Figure *figure);                       // draw tooltip data
static void draw_current_time(Figure* figure);  //show current time. might be used if the app is doing nothing
static void load_font(Figure* figure);

  typedef struct {
    size_t len,rows,cols,*skel;
    char* labels;
  }Skel;

#define SKEL_ERROR(format,...)do{			\
    fprintf(stderr,"%s:%d: ",__FILE__,__LINE__);	\
    fprintf(stderr,format ,##__VA_ARGS__);exit(1);}	\
  while(0)

static Skel skel_create(char*skel_str){
  if(!skel_str)SKEL_ERROR("Cannot be null\n");
  struct Cskel{
    uint32_t sx,sy,w,h;
    struct Cskel *next;
    char c;
  };
  uint32_t row,cols,col;
  row=cols=col=0;
  uint32_t index=0;
  Str str=string_create_from_format(0,NULL,"%s",skel_str);
  if(string_len(str)==0){SKEL_ERROR("len of str is 0\n");}
  char c;
  uint32_t col_start=0;
  while(index<string_len(str)){
    c=string_at(str,index);
    if(c==' '){
      uint32_t spaces=string_count_consecutive(str,' ',index);
      index+=spaces-1;
      col_start=index+1;
      col=0;
    }
    else{
      if(col==0)row+=1;
      bool k;
      if((((k=(index+1!=string_len(str))) && string_at(str,index+1)==' ') ||(!k))){
	if(cols==0){
	  cols=(index-col_start)+1;
	}
	else if((index-col_start)+1!=cols)
	  {SKEL_ERROR("unequal cols; expected %d got %d in row %d \n",cols,(index-col_start)+1,row);}	
      }
      col+=1;
    }
    index+=1;
  }
  Str str2=string_create(row*cols,NULL);
  index=0;
  uint32_t sindex=0;
  while(index<string_len(str)){
    if((c=string_at(str,index))!=' '){
      *string_pat(str2,sindex)=c;
      sindex+=1;
    }
    index+=1;}
  string_destroy(str);
  str=str2;
  struct Cskel* cskel=cm_malloc(sizeof(struct Cskel),"cskel");
  *cskel=(struct Cskel){0};
  struct Cskel *temp=cskel;    
  index=0;
  uint32_t unique_chars=1;
  for(size_t srow=0;srow<row;++srow){
    str2=string_split(str,srow*cols,cols);	
    for(size_t scol=0;scol<cols;++scol){
      c=string_at(str2,scol);
      uint32_t cc=string_count_consecutive(str2,c,scol);
      uint32_t ca=string_count(str2,c);
      if(cc!=ca)SKEL_ERROR("skipped occurence; consecutive col occurence of char %c is %d but total occurence is %d in row %zu\n",c,cc,ca,srow);
      struct Cskel otemp=(struct Cskel){.sx=scol,.sy=srow,.w=cc,.h=1,.c=c,.next=NULL};
      temp=cskel;
      while(1){
	if(temp->c==c){
	  if(srow>temp->sy+temp->h-1){
	    SKEL_ERROR("skipped row occurence for char '%c'; last valid row occurence at row %d col %d new one at row %zu col %zu\n",c,temp->sy+temp->h-1,temp->sx+temp->w-1,srow,scol);
	  }
	  goto NEXT;
	}
	if(temp->c==0){
	  *temp=otemp;
	  break;
	}
	else
	  {
	    if(!temp->next){
	      CM_MALLOC(sizeof(struct Cskel),temp->next);
	      *temp->next=otemp;
	      temp=temp->next;
	      unique_chars+=1;
	      break;
	    }
	    temp=temp->next;
	  }
      }
      Str str3=string_create(row,NULL);
      for(size_t i=0;i<row-srow;++i){
	*string_pat(str3,i)=string_at(str,(i+srow)*cols+scol);
      }
      cc=string_count_consecutive(str3,c,0);
      ca=string_count(str3,c);
      string_destroy(str3);
      if(cc!=ca)SKEL_ERROR("skipped occurence; consecutive row occurence of char %c is %d but total occurence is %d along col %zu\n",c,cc,ca,scol);
      temp->h=cc;
      if(cc!=1){
	for(size_t l=temp->sy+1;l<cc+temp->sy;++l){
	  Str s4=string_split(str,l*cols,cols);
	  uint32_t ll=string_count_consecutive(s4,c,scol);
	  uint32_t lc=string_count(s4,c);
	  if(ll!=temp->w||ll!=lc)SKEL_ERROR("unequal col occurences for char '%c'; %d occurences in row %d but %d occurence in row %zu\n",c,temp->w,temp->sy,ll==temp->w?lc:ll,l);
	  string_destroy(s4);
	}
      }
    NEXT:
      scol+=temp->w-1;
    }
    string_destroy(str2);
  }
  CM_MALLOC(sizeof(size_t)*4*unique_chars,size_t *ld);
  CM_MALLOC(sizeof(char)*unique_chars,char *cs);
  temp=cskel;
  while(temp){
    ld[0]=temp->sx;
    ld[1]=temp->sy;
    ld[2]=temp->w;
    ld[3]=temp->h;
    *cs=temp->c;
    ld+=4;
    cs+=1;
    struct Cskel*p=temp;
    temp=temp->next;	
    cm_free_ptrv(&p);
  }
  ld-=4*unique_chars;
  cs-=unique_chars;
  return (Skel){.len=unique_chars,.rows=row,.cols=cols,.skel=ld,.labels=cs};
} 

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
  case SCREEN_DIMENSION_STATE_CHANGED: 
    size_t axes_skels_copy[4] = {0};
    long int x= (long int)figure->height-(figure->font_size*(figure->title == NULL ?1 : 2)); // may underflow if height<font_size
    if(x<0){return false;}
    size_t axes_sizes[] = {figure->width / (figure->cols),  x / figure->rows},b0;
    for (size_t axes_index = 0; axes_index < figure->axes_len; axes_index++) {
      size_t index = axes_index * 4;
      // set new absolute measurements
      axes_skels_copy[0] = axes_sizes[0] * figure->axes_skels[index];
      axes_skels_copy[1] = axes_sizes[1] * figure->axes_skels[index + 1] + (figure->title == NULL ? 0 :figure->font_size);
      axes_skels_copy[2] = axes_sizes[0] * figure->axes_skels[index + 2];
      axes_skels_copy[3] = axes_sizes[1] * figure->axes_skels[index + 3];
      // ajdust the measurents  by removing border, creating space at the top for title height if title!=null   and
      // creating space at the bottom for tooltip
      b0 =  (size_t)(figure->border_dimensions[0] / 2);
      axes_skels_copy[0] += b0;
      axes_skels_copy[2] -=b0*2;
      b0 = (size_t)(figure->border_dimensions[1] / 2);
      axes_skels_copy[1] += b0;
      axes_skels_copy[3] -= b0*2;
      // then copy these measurements into the axes
      figure->axes[axes_index].startX = figure->axes_skels_copy[index] = axes_skels_copy[0];
      figure->axes[axes_index].startY = figure->axes_skels_copy[index + 1] = axes_skels_copy[1];
      figure->axes[axes_index].width = figure->axes_skels_copy[index + 2] = axes_skels_copy[2];
      figure->axes[axes_index].height = figure->axes_skels_copy[index + 3] = axes_skels_copy[3];
    }
    break;
  }
  return true;
}


static bool set_borders(Figure *figure) {
  figure->border_dimensions[0] = (figure->border_percentage * figure->width) / (figure->cols + 1);
  long int x=(long int)figure->height-(figure->font_size * (figure->title == NULL ? 1: 2));
  if(x<0){return false;}
  figure->border_dimensions[1] = (figure->border_percentage * x) / (figure->rows + 1);
  return true;
}

  
void raylib_init(Figure *figure){
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  SetTraceLogLevel(LOG_ERROR);
  char* window_title=figure->window_title?figure->window_title:string_create_from_format(0,NULL,"%s.%d",RAY_WINDOW_TITLE,getpid());
  InitWindow(figure->width, figure->height,window_title);
  string_destroy(window_title);
  load_font(figure);
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

static void draw_title(Figure *figure) {
  RC_ASSERT(figure->title!=NULL);
  float x = align_text(figure->font, figure->title, figure->width, figure->font_size, figure->font_spacing, RC_ALIGNMENT_CENTER);
  DrawTextEx(*(Font *)figure->font, figure->title, (Vector2){x, 0}, figure->font_size, figure->font_spacing, figure->text_color);
}

static void draw_tooltip(Figure *figure) {
  /*
    fps  [index_under_mouse/total_length] [current_time-xindex[-1]] timeframe 'axes_label_under_mouse' x_index_under_mouse y_index_under_mouse
  */
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


static void load_font(Figure* figure){;
  Font font = GetFontDefault();
  if(string_len(figure->font_path)){
    font = LoadFontEx(figure->font_path, figure->font_size, 0, 250);
    if (font.baseSize != figure->font_size) {      
      RC_WARN("could not load %s'; using default font\n",figure->font_path);
    }
  }
  *((Font *)figure->font) = font;
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
  draw_tooltip(figure);
  return true;
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

Figure *create_figure(char* figskel,int *fig_size, char *window_title, Rc_Color background_color,float border_percentage, int fps, size_t font_size, int font_spacing,char* font_path){
  Skel s=skel_create(figskel);
  if (border_percentage < 0.f || border_percentage >= 1.f) {RC_WARN("border_percentage is %f\n", border_percentage);}
  if (font_size == 0) {RC_ERROR("font_size is 0\n");}
  setlocale(LC_NUMERIC, "");
  size_t *border_dimensions = cm_malloc(sizeof(size_t) * 2, RC_ECHO(border_dimensions));
  size_t *axes_skels_dyn = cm_malloc(sizeof(size_t) * s.len * 4, RC_ECHO(axes_skels_dyn));
  memcpy(axes_skels_dyn, s.skel, sizeof(s.skel[0]) * s.len * 4);
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
      .axes_len = s.len,
      .rows = s.rows,
      .cols = s.cols,
      .axes_skels = axes_skels_dyn,
      .axes_skels_copy = s.skel,
      .axes = NULL,
      .border_dimensions = border_dimensions,
      .font = cm_malloc(sizeof(Font), RC_ECHO(Font)),
      .font_path=string_create_from_format(0,NULL,"%s",font_path),
      .mouseinfo = (MouseInfo){.down = false, .wait_up = false},
      .axes_frame_color = BLACK,
      .background_color = background_color,
      .text_color = BLACK,
      .sds = SCREEN_DIMENSION_STATE_DEFAULT,
      .show_cursors = false,
      .force_update=true,
      .has_dragger=false,
      .clear_screen=false,
  };
  create_axes(figure, s.labels);
  cm_free_ptrv(&s.labels);
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

