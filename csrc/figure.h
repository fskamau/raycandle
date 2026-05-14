#include "raylib.h"
#include "utils.h"

typedef struct {
  char* window_title;
  Arena* arena;
}Figure;

#define MAX_WINDOW_TITLE 256 // max window title char
#define FIGURE_ARENA_SIZE 1024*1024*3

Figure* figure_create(char *window_title, float spacing);
void figure_show(Figure* figure);
