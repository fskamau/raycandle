#include "utils.h"

int main(){
  Arena* arena=arena_create(1024);
  arena_print(arena);
  arena_alloc(arena, 2300);
  char*  ms;
  ms=str_create_with_arena(arena, 256);
  //  ms=str_create(NULL,256);
  
  str_summarize(ms);
  str_append(ms,"%s %d","");
  str_summarize(ms);
  printf("%s\n",ms);
  return 0;
}
