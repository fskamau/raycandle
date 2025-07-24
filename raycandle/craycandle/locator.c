#include "raycandle.h"
#include "utils.h"
#include <stdio.h>
#include <time.h>
#include "artist.h"

void locator_update_data_buffers(Axes* axes,LimitChanged lim){
  if(lim==LIMIT_CHANGED_ALL_LIM){RC_ASSERT(axes->artist_len!=0&&axes->artist!=NULL);}
  size_t rows=axes->parent->dragger.visible_data;
  if(rows==0){RC_ERROR("unreachable function '%s'  when figure has no xdataa\n",RC_ECHO(locator_update_data_buffers));}
  if(lim==LIMIT_CHANGED_XLIM||lim==LIMIT_CHANGED_ALL_LIM){
    double* xdata=axes->parent->dragger.xdata_shared;
    size_t width=axes->width;
    size_t startX=axes->startX;
    for(size_t row=0;row<rows;row++){
      axes->xdata_buffer[row]=xdata[row]*width+startX;
      }
  }
  for(size_t i=0;i<axes->artist_len;++i){
    artist_update_data_buffer(get_artist(axes, i),lim);
  }
}

#include <string.h>
void epoch2strftime(int epoch,Str buffer,const char* format){
  unsigned int len_buffer;
  int written;
  RC_ASSERT(format!=NULL);
  time_t raw_time=epoch;
  if(epoch==0){time(&raw_time);}
  struct tm tm=*localtime(&raw_time);
  string_increase_len(buffer,written=strftime(string_get_current_point(buffer),(len_buffer=string_get_remaining(buffer)),format,&tm));
  if(written==0){
      fprintf(stderr,"strftime(%s) would exceed buffer size of %d bytes.\n",format,len_buffer);
      cm_free_all();
      exit(1);
  }
}


/*
put the data under mouse position on the tooltip
*/
void locator_tooltip_mouse_position(Axes* axes,Str buffer,int mouseX,int mouseY){
    RC_ASSERT(axes->artist_len!=0 && axes->parent->dragger.len_data!=0);
    if(axes->ylocator.format==NULL&&axes->ylocator.locator_type!=FORMATTER_NULL_FORMATTER){RC_ERROR("formatter is not null but format is null\n");}
    double x=RC_PIXEL_X_2_DATA((float)mouseX,axes);
    double y=RC_PIXEL_Y_2_DATA((float)mouseY,axes);
    string_append(buffer,"x=");
    formatter_to_str(axes->parent->dragger.xformatter, axes->parent->dragger.xlim_format,buffer,&x);
    string_append(buffer," y=");
    formatter_to_str(axes->ylocator.locator_type,axes->ylocator.format,buffer,&y);
}

void formatter_to_str(Formatter formatter,Str format,Str buffer,void* mem){
  switch (formatter) {
    case FORMATTER_NULL_FORMATTER:
      string_append(buffer,"<null>");
      return; 
  case FORMATTER_LINEAR_FORMATTER:
    string_append(buffer,format,*(double*)mem);//Only doubles allowed
      return;
    case FORMATTER_TIME_FORMATTER:
      epoch2strftime(*(double*)mem,buffer,format);
      return;
  }
  RC_ERROR("implement formatter to string for FORMAT id %d\n",formatter);
}
