#include <stdlib.h>
#include "raycandle.h"

#define CM_SILENT !RAYCANDLE_DEBUG
#define CM_IMPLEMENTATION
#include "cust_malloc.h"
#define CS_IMPLEMENTATION
#include "cs_string.h"
#include "utils.h"

static Log_Level RC_Log_Level_Min=LOG_LEVEL_INFO;


void RC_write_std(Log_Level log_level,const char file[],int line,const char func[],const char error_format[],...){
  if(log_level!=LOG_LEVEL_INFO && log_level!=LOG_LEVEL_WARN&&log_level!=LOG_LEVEL_ERROR){RC_ERROR("Unkown log level %d\n",log_level);}
  if(log_level<RC_Log_Level_Min&&log_level!=LOG_LEVEL_ERROR){return;} //we allow errors to pass
  char _error_buffer[RAYCANDLE_MAX_ERROR_STR] = {0};
  char* error_buffer=string_create(RAYCANDLE_MAX_ERROR_STR,_error_buffer);
  va_list args;
  va_start(args,error_format);
  char* log_type=(log_level)==LOG_LEVEL_INFO?"Info":(log_level==LOG_LEVEL_WARN)?"Warn":"Error";
  
#if RAYCANDLE_DEBUG //file:line log_type[time] func message
  string_append(error_buffer,"%s:%d:[%s]: in function '%s': ", file, line,log_type, func);
#else //Info [time] message
  (void)file;
  (void)line;
  (void)func;
  string_append(error_buffer,"%s : ",log_type);
#endif
  string_append(error_buffer,"%s",error_format);
  vfprintf(log_level==LOG_LEVEL_INFO?stdout:stderr,error_buffer, args);
  va_end(args);
  if((log_level)==LOG_LEVEL_ERROR){
    cm_free_all();
    exit(EXIT_FAILURE);
    }
}

float align_text(Font* font,const char * text, int width,int font_size,int spacing,Rc_Alignment alignment){
  float f=MeasureTextEx(*font,text,font_size,spacing).x;
  if(f==0.f){f=MeasureText(text, font_size);}
  if((float)width<f){return 0.f;}
  switch(alignment){
  case RC_ALIGNMENT_CENTER:
    return (width -f)/ 2;
  case RC_ALIGNMENT_LEFT:
    return 0.f;
  case RC_ALIGNMENT_RIGHT:
    return width -f;
  }
  RC_ERROR("non reachable condition\n");
  return 0.f;
}

Color RC_faded_color_from(Color original) {
  uint8_t fade_amount=70;
    Color faded;
    faded.a = (original.a > fade_amount) ? (original.a - fade_amount) : fade_amount;
    faded.r = (original.r > fade_amount) ? (original.r -fade_amount ) : fade_amount;
    faded.g = (original.g > fade_amount) ? (original.g -fade_amount ) : fade_amount;
    faded.b = (original.b > fade_amount) ? (original.b -fade_amount ) : fade_amount;
    return faded;
}

Color axes_get_next_tableau_t10_color(Axes* axes){
  RC_ASSERT(axes->tableau_t10_index>=0&&axes->tableau_t10_index<10);
  Color color =tableau_colors[(axes->tableau_t10_index++)];
  axes->tableau_t10_index%=10;
  return color;
}

void cm_free_all_(void){
  cm_free_all();
}
