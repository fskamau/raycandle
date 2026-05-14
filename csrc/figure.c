#include "figure.h"
#include "utils.h"

Figure* figure_create(char *window_title, float spacing){
  Arena* arena=arena_create((size_t)FIGURE_ARENA_SIZE);
  arena_print(arena);
  Figure* figure=arena_alloc_or_die(arena,sizeof(Figure));
  
  return figure;
}
void figure_show(Figure* figure);
