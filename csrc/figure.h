#include "raylib.h"
#include "utils.h"

typedef struct {
  char* title;
  Arena* arena;
  float spacing;
}Figure;

#define MAX_FIGURE_TITLE 256 // max figure title char
#define FIGURE_ARENA_SIZE 1024*1024*3

Figure* figure_create();
void figure_set_title(Figure* figure, char *title);
void figure_show(Figure* figure);
