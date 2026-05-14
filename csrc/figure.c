#include "figure.h"
#include "utils.h"
#include <string.h>

Figure* figure_create(){
  Arena* arena=arena_create((size_t)FIGURE_ARENA_SIZE);
  Figure* figure=arena_alloc_or_die(arena,sizeof(Figure));
  memset(figure,0,sizeof(Figure));
  figure->arena=arena;
  return figure;
}


void figure_set_title(Figure* figure, char *title)
{
  if(!title||!figure)ray_error("null argument figure or title");
  if(!figure->title)
    figure->title=str_create_with_arena(figure->arena, MAX_FIGURE_TITLE);
  str_clear(figure->title);
  str_append(figure->title, "%s",title);
}
  
void figure_show(Figure* figure);
