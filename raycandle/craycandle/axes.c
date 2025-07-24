#include "artist.h"
#include "utils.h"

static bool adjust_axes_dimensions(Axes *axes);//plots the axes frames and ajusts the axes dimensions
static void axes_draw_legend(Axes *axes);


static bool adjust_axes_dimensions(Axes *axes){
  //adjust for frame
  axes->startX+=AXES_FRAME_THICK;
  axes->startY+=AXES_FRAME_THICK;
  float temp;
  axes->startY+=(temp=(axes->height*axes->padding));
  axes->height-=temp*2;
  axes->startX+=(temp=(axes->width*axes->padding));
  axes->width-=temp*2;
  if((long int)(axes->width-=(AXES_FRAME_THICK*2))<0||(long int)(axes->height-=(AXES_FRAME_THICK*2))<0){
    return false;
  }
  //adjust for title
  if(axes->title!=NULL){
    axes->startY+=axes->parent->font_size;
    return  (long int)(axes->height-=axes->parent->font_size)>0;
  }
  return true;
}

static void axes_draw_legend(Axes *axes){
  int startx=0,starty=0,paddingdiv=RC_LEGEND_PADDING/3;
  switch (axes->legend.legend_position) {
    case LEGEND_POSITION_TOP_LEFT:
      startx=axes->startX;
      starty=axes->startY;
      break;
    case LEGEND_POSITION_TOP_RIGHT:
      startx=axes->width-axes->legend.width+axes->startX-RC_LEGEND_PADDING;
      starty=axes->startY;
      break;
    case LEGEND_POSITION_BOTTOM_LEFT:
      startx=axes->startX;
      starty=axes->height-axes->legend.height+axes->startY;
      break;
    case LEGEND_POSITION_BOTTOM_RIGHT:
      startx=axes->width-axes->legend.width+axes->startX-RC_LEGEND_PADDING;
      starty=axes->height-axes->legend.height+axes->startY;
      break;
    case LEGEND_POSITION_NO_LEGEND:
      break;
    default:
      RC_ERROR("unreachable condition! %d %d %d\n",axes->legend.legend_position,startx,starty);
  }
  
  DrawRectangleLinesEx((Rectangle){startx, starty, axes->legend.width+RC_LEGEND_PADDING, axes->legend.height},2,RC_faded_color_from(axes->facecolor));
  startx+=paddingdiv;
  for(size_t i=0;i<axes->artist_len;++i){
    Artist* artist=get_artist(axes, i);
    char* label=artist->gdata.label;
    if(label==NULL){continue;}
    artist_draw_icon(artist,(Vector2){.x=startx,.y=starty});
    DrawTextEx(FIGURE_FONT(axes->parent),label,(Vector2){startx+RC_LEGEND_ICON_WIDTH+paddingdiv,starty},axes->parent->font_size,axes->parent->font_spacing,axes->parent->text_color);
    starty+=axes->parent->font_size;
  }
}

void create_axes(Figure *figure,char* labels){
  figure->axes=cm_malloc(sizeof(Axes)*figure->axes_len,RC_ECHO(axes));
  for (size_t i=0;i<figure->axes_len;++i){
    figure->axes[i]=(Axes){
    .xdata_buffer=NULL,
    .parent=figure,
    .title=NULL,
    .artist_len=0,
    .artist=NULL,
    .label=labels[i],
    .ylocator=(Locator){.format="%.10f",.flen=11,.limit=(Limit){.is_static=false},.locator_type=FORMATTER_LINEAR_FORMATTER},
    .legend=(Legend){.legend_position=LEGEND_POSITION_NO_LEGEND,.height=0,.width=0},
    .facecolor=figure->background_color,
    .tableau_t10_index=0,
    .padding=0.f,
    };
  }
}

bool draw_axes(Axes* axes){
  DrawRectangleLinesEx((Rectangle){axes->startX,axes->startY,axes->width,axes->height},AXES_FRAME_THICK,axes->parent->axes_frame_color); // draw frame first before ajusting
  if(!adjust_axes_dimensions(axes)){return false;}
  if(!RC_COLOR1_EQUALS_COLOR2(axes->facecolor,axes->parent->background_color)){DrawRectangle(axes->startX,axes->startY,axes->width,axes->height,axes->facecolor);} //save some fps
  if(axes->legend.legend_position!=LEGEND_POSITION_NO_LEGEND){axes_draw_legend(axes);}
  if (axes->title!=NULL){
    float x=align_text(axes->parent->font,axes->title,axes->width,axes->parent->font_size,axes->parent->font_spacing,RC_ALIGNMENT_CENTER);
    DrawTextEx(FIGURE_FONT(axes->parent),axes->title,(Vector2){axes->startX+x,axes->startY-axes->parent->font_size},axes->parent->font_size,axes->parent->font_spacing,axes->parent->text_color);
    DrawLineEx((Vector2){axes->startX,axes->startY},(Vector2){axes->startX+axes->width,axes->startY},AXES_FRAME_THICK,axes->parent->axes_frame_color);
  }
  if(axes->parent->dragger.len_data!=0&&axes->artist_len>0){
    if(axes->parent->sds==SCREEN_DIMENSION_STATE_CHANGED){
      locator_update_data_buffers(axes,LIMIT_CHANGED_ALL_LIM);
      }
    for(size_t i=0;i<axes->artist_len;++i){draw_artist(get_artist(axes, i));};
  }
  return true;
}

size_t get_axes_index_under_mouse(Figure* figure){
  size_t x=GetMouseX();
  size_t y=GetMouseY();
  for(size_t i=0;i<figure->axes_len;++i){
    Axes axes=figure->axes[i];
    if (axes.width+axes.startX>x&&x>axes.startX && axes.height+axes.startY>y&&y>axes.startY){return i;}
  }
  return figure->axes_len;
}

Axes* get_axes_under_mouse(Figure* figure){
  size_t aium=get_axes_index_under_mouse(figure);
  if (aium==figure->axes_len)return NULL;
  return figure->axes+aium;
}
RC_ONCE_AFTER_INIT_WINDOW  void axes_set_legend(Figure* figure){
  for(size_t i=0;i<figure->axes_len;++i){
    Axes* axes=figure->axes+i;
    for(size_t a=0;a<axes->artist_len;++a){
      char* label=get_artist(axes, a)->gdata.label;
      if(label==NULL){continue;;}
      axes->legend.height+=figure->font_size;
      axes->legend.width=RC_MAX(axes->legend.width,(int)MeasureTextEx(FIGURE_FONT(axes->parent),label,axes->parent->font_size,axes->parent->font_spacing).x);
    }
    axes->legend.width+=RC_LEGEND_ICON_WIDTH;
  }
}

void axes_set_yformatter(Axes* axes,char*formatter){
  RC_ERROR("TODO: format actual types \n");
  if(axes->ylocator.format!=NULL){string_destroy((axes->ylocator.format));}
  axes->ylocator.format=string_create_from_format(0,NULL,formatter);
}

#define RC_AXES_MAX_TITLE 256

void axes_set_title(Axes* axes,char * title){
  if(axes->title==NULL)axes->title=string_create(RC_AXES_MAX_TITLE,NULL);
  else string_clear(axes->title);
  string_append(axes->title,title);
}

void axes_show_legend(Axes* axes,LegendPosition legend_position){
  switch (legend_position) {
    case LEGEND_POSITION_BOTTOM_LEFT:
    case LEGEND_POSITION_BOTTOM_RIGHT:
    case LEGEND_POSITION_TOP_RIGHT:
    case LEGEND_POSITION_TOP_LEFT:
      axes->legend.legend_position=legend_position;
      break;
    case LEGEND_POSITION_NO_LEGEND:
    default:
      RC_ERROR("uknown legend position '%d'\n",legend_position);
      break;
  }
}

void axes_set_ylim(Axes* axes,double yminmax[2]){
  if(axes->parent->dragger.len_data==0){RC_ERROR("cannot set xlim for axes since *figure* has 0 artists \n");}
  if(axes->artist_len==0){RC_WARN("Axes has no artists and hence '%s' has no effect\n",RC_ECHO(axes_set_ylim));}
  axes->ylocator.limit.is_static=true;
  axes->ylocator.limit.limit_min=yminmax[0];
  axes->ylocator.limit.limit_max=yminmax[1];
  axes->ylocator.limit.diff=yminmax[1]-yminmax[0];
  locator_update_data_buffers(axes, LIMIT_CHANGED_YLIM);
}
